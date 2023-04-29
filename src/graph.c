#include "graph.h"
#include "config.h"
#include "dialog.h"
#include "util.h"

/// Context for drawing a single graph.
typedef struct {
    Canvas *const canvas;
    const Rectangle viewport;
    const unsigned scale;
    const bool fill;
    short color;
    /** Left point, older sample */
    double x1, y1;
    /** Right point, newer sample */
    double x2, y2;
} GraphDrawContext;

typedef void(*DrawLineFn)(const GraphDrawContext *restrict);

static void DrawLine(const GraphDrawContext *restrict ctx) {
    if (false && ctx->fill) {
        // FIXME: this is invalid when the viewport does not stretch to the
        // bottom of the canvas.
        CanvasDrawLineFill(ctx->canvas, ctx->x1, ctx->y1, ctx->x2, ctx->y2, ctx->color);
    } else {
        CanvasDrawLine(ctx->canvas, ctx->x1, ctx->y1, ctx->x2, ctx->y2, ctx->color);
    }
}

static void DrawBezir(const GraphDrawContext *restrict ctx) {
    // Create locals for the values used in the loop to prevent the repeated
    // memory indirection. Not sure if this actually does anything but it
    // shouldn't hurt either.
    Canvas *const canvas = ctx->canvas;
    // Flipping the 1 and 2 points is intentional here because of how this
    // used to be called.
    const double x1 = ctx->x1;
    const double y1 = ctx->y1;
    const double x2 = ctx->x2;
    const double y2 = ctx->y2;
    const short color = ctx->color;
    if ((x2 - x1) < 1.0 || fabs(y1 - y2) < 1.0) {
        CanvasDrawLine(canvas, x1, y1, x2, y2, color);
        return;
    }
    // TODO: we still get holes in these sometimes, should probably reduce the
    // number of points we calculate and then draw straight lines between those.
    /* Arbitrary constant that defined how squiggly(?) the bezir curve will be;
       0.0 -> straight line, 2.0 -> very shallow at beginning, vertical in the
       middle. Values less than 0.0 or greater than 2.0 will cause it to overlap
       itself. */
    #define CONTROL_FACTOR ((2.0 + 1.618) / 3.0)
    const double dx = (x2 - x1) * 2.0;
    const double dy = fabs(y1 - y2);
    const double percision = 0.9 / (dx > dy ? dx : dy);
    /* Control point coordinates.
       Invariant: x2 > x1 */
    const double x1_c = x1 + ctx->scale * CONTROL_FACTOR;
    const double x2_c = x2 - ctx->scale * CONTROL_FACTOR;
    const double y1_c = y1;
    const double y2_c = y2;
    const double bottom = ctx->viewport.height * 4.0 - 1.0;
    double d, d2, d3, w1, w2, w3, w4, x, y;
    for (d = 0.0; d <= 1.0; d += percision) {
        // clang-format off
        d2 = d * d;
        d3 = d2 * d;
        w1 =   -d3 + 3*d2 - 3*d + 1;
        w2 =  3*d3 - 6*d2 + 3*d    ;
        w3 = -3*d3 + 3*d2          ;
        w4 =    d3                 ;
        x = x1*w1 + x1_c*w2 + x2_c*w3 + x2*w4;
        if (x < 0.0) {
            continue;
        }
        y = y1*w1 + y1_c*w2 + y2_c*w3 + y2*w4;
        // clang-format on
        if (ctx->fill) {
            for (double yy = bottom; yy >= y; --yy) {
                CanvasSet(ctx->canvas, x, yy, ctx->color);
            }
        } else {
            CanvasSet(canvas, x, y, color);
        }
    }
}

static void DrawBlocks(const GraphDrawContext *restrict ctx) {
    if (ctx->fill) {
        // See `GraphDraw` about the `- 1`.
        const double bottom = (ctx->viewport.y - 1 + ctx->viewport.height) * 4.0 - 1.0;
        CanvasDrawRect(ctx->canvas, ctx->x1, bottom, ctx->x2 - 1, ctx->y2, ctx->color);
    } else {
        for (double xx = Max(0.0, ctx->x1); xx <= ctx->x2; ++xx) {
            CanvasSet(ctx->canvas, xx, ctx->y2, ctx->color);
        }
        if (ctx->x1 >= 0.0) {
            const double y1 = Min(ctx->y1, ctx->y2);
            const double y2 = Max(ctx->y1, ctx->y2);
            for (double yy = y1; yy <= y2; ++yy) {
                CanvasSet(ctx->canvas, ctx->x1, yy, ctx->color);
            }
        }
    }
}

static DrawLineFn GraphKindLineDrawer(Graph_Kind kind) {
    switch (kind) {
        case GRAPH_KIND_STRAIGHT: return DrawLine;
        case GRAPH_KIND_BEZIR: return DrawBezir;
        case GRAPH_KIND_BLOCKS: return DrawBlocks;
    }
    __builtin_unreachable();
}

static void GraphKindFromString(const char *s, Graph_Kind *out) {
    static const struct {
        const char *s;
        Graph_Kind k;
    } VALID_PAIRS[] = {
        {"straight", GRAPH_KIND_STRAIGHT},
        {"bezir", GRAPH_KIND_BEZIR},
        {"blocks", GRAPH_KIND_BLOCKS}
    };
    for (size_t i = 0; i < countof(VALID_PAIRS); ++i) {
        if (strcasecmp(s, VALID_PAIRS[i].s) == 0) {
            *out = VALID_PAIRS[i].k;
        }
    }
    fprintf(stderr, "invalid graph kind: %s\n", s);
}

void GetGraphOptions(const char *domain, Graph_Kind *kind_out, unsigned *scale_out) {
    if (!HaveConfig ())
        return;
    Config_Read_Value *v;
    if ((v = ConfigGet(domain, "graph-kind"))) {
        GraphKindFromString(v->as_string(), kind_out);
    }
    if ((v = ConfigGet(domain, "graph-scale"))) {
        *scale_out = v->as_unsigned();
    }
}

void GraphConstruct(Graph *self, Graph_Kind kind, size_t n_sources, unsigned scale) {
    self->kind = kind;
    self->samples = calloc(n_sources, sizeof(List *));
    for (size_t i = 0; i < n_sources; ++i)
        self->samples[i] = list_create();
    self->n_sources = n_sources;
    self->max_samples = 1;
    self->scale = scale;
    self->viewport = (Rectangle){ 0, 0, 0, 0 };
    self->lowest_sample = 0.0;
    self->highest_sample = 1.0;
    self->range_step = 0.1;
    self->fixed_range = true;
    self->set_colors = NULL;
}

void GraphDestroy(Graph *self) {
    for (size_t i = 0; i < self->n_sources; ++i)
        list_delete(self->samples[i]);
    free(self->samples);
    vector_free(self->set_colors);
}

static void GraphSetMaxSamples(Graph *self, size_t max_samples) {
    if (max_samples >= self->max_samples) {
        self->max_samples = max_samples;
        return;
    }
    for (size_t i = 0; i < self->n_sources; ++i)
        list_shrink(self->samples[i], max_samples);
    self->max_samples = max_samples;
}

void GraphSetViewport(Graph *self, Rectangle viewport) {
    // We need 1 extra sample as the scale equation only gives us the number of
    // line segments we need which is 1 less than the number of samples.
    GraphSetMaxSamples(self, 1 + (viewport.width + self->scale - 1) / self->scale);
    self->viewport = viewport;
}

void GraphSetFixedRange(Graph *self, double lo, double hi) {
    self->fixed_range = true;
    self->lowest_sample = lo;
    self->highest_sample = hi;
}

static double GraphApplyStep(double sample, double step, bool up) {
    // FIXME: doesn't work correctly with negative samples.
    return round(sample / step) * step + up * step;
}

static void GraphGetRange(Graph *self, double *lo_out, double *hi_out) {
    double s;
    double lo = self->samples[0]->back->f;
    double hi = lo;
    for (size_t i = 0; i < self->n_sources; ++i) {
        list_for_each (self->samples[i], it) {
            s = it->f;
            if (s > hi)
                hi = s;
            else if (s < lo)
                lo = s;
        }
    }
    *lo_out = GraphApplyStep(lo, self->range_step, false);
    *hi_out = GraphApplyStep(hi, self->range_step, true);
}

void GraphSetDynamicRange(Graph *self, double step) {
    self->fixed_range = false;
    self->range_step = step;
}

void GraphSetScale(Graph *self, unsigned scale, bool update_max_samples) {
    self->scale = scale;
    if (update_max_samples) {
        GraphSetViewport(self, self->viewport);
    }
}

void GraphAddSample(Graph *self, size_t source, double sample) {
    List *list = self->samples[source];
    if (likely(list->count == self->max_samples)) {
        list_rotate_left(list)->f = sample;
    } else {
        list_push_back(list)->f = sample;
    }
}

short GraphSourceColor(Graph *self, int source) {
    enum { N_DEFAULT = 8 };
    static const short DEFAULT_FIRST[N_DEFAULT] = {5, 4, 3, 2, 6, 7, 8, 9};
    if (source < (int)vector_size(self->set_colors)) {
        return self->set_colors[source];
    } else if (source < N_DEFAULT) {
        return DEFAULT_FIRST[source];
    } else {
        return 18 + (37 * source) % 214;
    }
}

void GraphDraw(Graph *self, Canvas *canvas, double *lo_out, double *hi_out) {
    #define Y(sample) (height - ((height - 0.25) * (sample)))
    double lowest_sample, highest_sample;
    if (self->fixed_range) {
        lowest_sample = self->lowest_sample;
        highest_sample = self->highest_sample;
    } else {
        GraphGetRange(self, &lowest_sample, &highest_sample);
        // FIXME: for now we don't need to scale to bottom end of the range
        // but for proper support we'd need to be able a fixed/dynamic value
        // for the start and end separately.
        lowest_sample = 0.0;
    }
    GraphDrawContext ctx = {
        canvas,
        self->viewport,
        self->scale,
        self->n_sources == 1,
        0,
        0.0,
        0.0,
        0.0,
        0.0,
    };
    const double scale = 2.0 * self->scale;
    const double top_y = (self->viewport.y - 1) * 4.0;
    const double height = self->viewport.height;
    const DrawLineFn DrawLine = GraphKindLineDrawer(self->kind);
    // This `- 1` is specific to how all the graphs are currently used but it
    // saves us from needing to differentiate between window space and canvas
    // space.
    double scaled_sample;
    for (int i = self->n_sources - 1; i >= 0; --i) {
        List *list = self->samples[i];
        if (list->count <= 1)
            continue;
        scaled_sample = (list->front->f - lowest_sample) / highest_sample;
        ctx.x1 = (double)self->viewport.width * 2.0 - (list->count - 1) * scale;
        ctx.y1 = top_y + Y(scaled_sample) * 4.0 - 1;
        ctx.x2 = ctx.x1;
        ctx.color = GraphSourceColor(self, i);
        for (const List_Node *it = list->front->next; it != NULL; it = it->next) {
            scaled_sample = (it->f - lowest_sample) / highest_sample;
            // x2,y2 are the current point because we want x1,y1 to be the
            // point on the left.
            ctx.x2 += scale;
            ctx.y2 = top_y + Y(scaled_sample) * 4.0 - 1;
            DrawLine(&ctx);
            ctx.x1 = ctx.x2;
            ctx.y1 = ctx.y2;
        }
    }
    if (lo_out) *lo_out = lowest_sample;
    if (hi_out) *hi_out = highest_sample;
}

void GraphSetColors(Graph *self, ...) {
    va_list ap;
    va_start(ap, self);
    short color = 0;
    while ((color = (short)va_arg(ap, int)) >= 0) {
        vector_push(self->set_colors, color);
    }
    va_end(ap);
}

double GraphLastSample(Graph *self, int source) {
    return self->samples[source]->back->f;
}

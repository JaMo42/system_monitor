#pragma once
#include "canvas/canvas.h"
#include "stdafx.h"
#include "vector.h"

typedef enum {
    GRAPH_KIND_STRAIGHT,
    GRAPH_KIND_BEZIR,
    GRAPH_KIND_BLOCKS,
} Graph_Kind;

typedef struct {
    int16_t x, y, width, height;
} Rectangle;

typedef struct {
    Graph_Kind kind;
    List **samples;
    size_t n_sources;
    size_t max_samples;
    unsigned scale;
    Rectangle viewport;
    double lowest_sample;
    double highest_sample;
    double range_step;
    bool fixed_range;
    VECTOR(short) set_colors;
} Graph;

void GraphConstruct(Graph *self, Graph_Kind kind, size_t n_sources, unsigned scale);
void GraphDestroy(Graph *self);
void GraphSetViewport(Graph *self, Rectangle viewport);
void GraphSetFixedRange(Graph *self, double lo, double hi);
void GraphSetDynamicRange(Graph *self, double step);
void GraphSetScale(Graph *self, unsigned scale, bool update_max_samples);
void GraphAddSample(Graph *self, size_t source, double sample);
void GraphDraw(Graph *self, Canvas *canvas, double *lo_out, double *hi_out);
/** Overwrites the default colors for any number of sources, terminated by -1. */
void GraphSetColors(Graph *self, ...);
short GraphSourceColor(Graph *self, int source);
double GraphLastSample(Graph *self, int source);

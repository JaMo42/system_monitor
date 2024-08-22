#pragma once
#include "canvas/canvas.h"
#include "stdafx.h"
#include "vector.h"

#define DEFAULT_GRAPH_SCALE 8

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

void
GetGraphOptions(const char *domain, Graph_Kind *kind_out, unsigned *scale_out);

void
GraphConstruct(Graph *self, Graph_Kind kind, size_t n_sources, unsigned scale);

void GraphDestroy(Graph *self);

/** Sets the area the graph will be drawn to. */
void GraphSetViewport(Graph *self, Rectangle viewport);

/** Sets the graph to use fixed scaling in the given range. */
void GraphSetFixedRange(Graph *self, double lo, double hi);

/** Sets the graph to use dynamic scaling, using the given increment. */
void GraphSetDynamicRange(Graph *self, double step);

/** Sets the horizontal scaling.  If `update_max_samples` is true this set the
    the maximum number of samples based on the width of the viewport. */
void GraphSetScale(Graph *self, unsigned scale, bool update_max_samples);

/** Adds a sample to the source. */
void GraphAddSample(Graph *self, size_t source, double sample);

/** Draws the graph.  The highest and lowest sample from all sources is written
    to the `lo_out` and `hi_out` parameters respectively, if they are not NULL.
 */
void GraphDraw(Graph *self, Canvas *canvas, double *lo_out, double *hi_out);

/** Overwrites the default colors for any number of sources, terminated by -1.
 */
void GraphSetColors(Graph *self, ...);

/** Overwrites the default colors for any number of sources. */
void GraphSetColorsList(Graph *self, short *colors, size_t n);

/** Returns the graph color for the given source.  This is either the value
    speified by `GraphSetColors` or the default. */
short GraphSourceColor(Graph *self, int source);

/** Returns the last sample added to the given source. */
double GraphLastSample(Graph *self, int source);

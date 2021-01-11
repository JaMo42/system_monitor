#pragma once
#include <stddef.h>
#include <stdint.h>
#include <ncurses.h>

typedef struct Canvas
{
  uint16_t *chars;
  short *colors;
  size_t width;
  size_t height;
} Canvas;

Canvas * CanvasCreate (size_t width, size_t height);
void CanvasDelete (Canvas *c);

void CanvasClear (Canvas *c);
void CanvasResize (Canvas *c, size_t width, size_t height);

void CanvasSet (Canvas *c, double x_, double y_, uint16_t color);
void CanvasSetText (Canvas *c, size_t x, size_t y, const char *text,
                    short color);

void CanvasDraw (const Canvas *c, WINDOW *win);

void CanvasDrawLine (Canvas *c, double x1_, double y1_, double x2_, double y2_,
                     short color);

/* Draw a line and fill the space below it */
void CanvasDrawLineFill (Canvas *c, double x1_, double y1_, double x2_,
                         double y2_, short color);

void CanvasDrawRect (Canvas *c, double x1_, double y1_, double x2_,
                     double y2_, short color);

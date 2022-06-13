#pragma once
#include <stddef.h>
#include <stdint.h>
#include <ncurses.h>

typedef struct Canvas
{
  uint8_t *chars;
  short *colors;
  size_t width;
  size_t height;
} Canvas;

Canvas * CanvasCreate (WINDOW *win);
void CanvasDelete (Canvas *c);

void CanvasClear (Canvas *c);
void CanvasResize (Canvas *c, WINDOW *win);

void CanvasSet (Canvas *c, double x_, double y_, short color);

void CanvasDraw (const Canvas *c, WINDOW *win);

void CanvasDrawLine (Canvas *c, double x1_, double y1_, double x2_, double y2_,
                     short color);

/* Draw a line and fill the space below it */
void CanvasDrawLineFill (Canvas *c, double x1_, double y1_, double x2_,
                         double y2_, short color);

void CanvasDrawRect (Canvas *c, double x1_, double y1_, double x2_,
                     double y2_, short color);


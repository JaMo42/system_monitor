#include "canvas.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Apparenly this isn't included by <ncurses.h> */
extern int waddnwstr(WINDOW *win, const wchar_t *wstr, int n);

#define BRAILLE_MASK 0xFF
#define TEXT_MASK 0xFF00

#define NORMALIZE(x) (size_t)round ((double)(x))

static const uint16_t pixel_map[4][2] = {
  {0x01, 0x08},
  {0x02, 0x10},
  {0x04, 0x20},
  {0x40, 0x80}
};

static const int braille_char_offset = 0x2800;

Canvas *
CanvasCreate (size_t width, size_t height)
{
  Canvas *c = (Canvas *)malloc (sizeof (Canvas));
  c->chars = NULL;
  c->colors = NULL;
  CanvasResize (c, width, height);
  return c;
}

void
CanvasDelete (Canvas *c)
{
  free (c->chars);
  free (c->colors);
  free (c);
}

void
CanvasClear (Canvas *c)
{
  //memset (c->chars, 0, c->width * c->height);
  for (size_t i = 0; i < c->width * c->height; ++i)
    {
      c->chars[i] = 0;
      c->colors[i] = -1;
    }
}

void
CanvasResize (Canvas *c, size_t width, size_t height)
{
  /* No need to preserve old content. */
  free (c->chars);
  free (c->colors);
  c->chars = calloc (width * height, sizeof (uint8_t));
  c->colors = malloc (width * height * sizeof (short));
  for (size_t i = 0; i < width * height; ++i)
    c->colors[i] = -1;
  c->width = width;
  c->height = height;
}

void
CanvasSet (Canvas *c, double x_, double y_, short color)
{
  const size_t x = NORMALIZE (x_);
  const size_t y = NORMALIZE (y_);
  const int col = x / 2;
  const int row = y / 4;
  if (col < 0 || col >= c->width || row < 0 || row >= c->height)
    return;
  const size_t idx = (x / 2) + (y / 4) * c->width;

  c->chars[idx] |= pixel_map[y % 4][x % 2];
  c->colors[idx] = color;
}

void
CanvasDraw (const Canvas *c, WINDOW *win)
{
  size_t row_off;
  wchar_t ch[] = { 0, 0 };
  uint16_t cell;

  for (size_t y = 0; y < c->height; ++y)
    {
      row_off = y * c->width;
      wmove (win, y + 1, 1);
      for (size_t x = 0; x < c->width; ++x)
        {
          ch[0] = braille_char_offset + (c->chars[row_off + x] & BRAILLE_MASK);
          wattron (win, COLOR_PAIR (c->colors[row_off + x]));
          waddnwstr (win, ch, 1);
          wattroff (win, COLOR_PAIR (c->colors[row_off + x]));
        }
    }
}

void
CanvasDrawLine (Canvas *c, double x1_, double y1_, double x2_, double y2_,
                short color)
{
  const size_t x1 = NORMALIZE (x1_);
  const size_t y1 = NORMALIZE (y1_);
  const size_t x2 = NORMALIZE (x2_);
  const size_t y2 = NORMALIZE (y2_);

  const size_t xd = abs (x1 - x2);
  const size_t yd = abs (y1 - y2);
  const int xs = x1 <= x2 ? 1 : -1;
  const int ys = y1 <= y2 ? 1 : -1;

  const double r = xd > yd ? xd : yd;

  double x, y;
  for (size_t i = 0; i <= r; ++i)
    {
      x = (double)x1;
      y = (double)y1;

      if (yd != 0)
        y += (double)(i * yd) / r * ys;
      if (xd != 0)
        x += (double)(i * xd) / r * xs;

      CanvasSet (c, x, y, color);
    }
}

#define CANVAS_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CANVAS_MIN(a, b) (((a) < (b)) ? (a) : (b))

void
CanvasDrawLineFill (Canvas *c, double x1_, double y1_, double x2_, double y2_,
                    short color)
{
  const size_t x1 = NORMALIZE (x1_);
  const size_t y1 = NORMALIZE (y1_);
  const size_t x2 = NORMALIZE (x2_);
  const size_t y2 = NORMALIZE (y2_);

  const size_t xd = abs (x1 - x2);
  const size_t yd = abs (y1 - y2);
  const int xs = x1 <= x2 ? 1 : -1;
  const int ys = y1 <= y2 ? 1 : -1;

  const double r = CANVAS_MAX (xd, yd);

  double x, y;
  for (size_t i = 0; i <= r; ++i)
    {
      x = (double)x1;
      y = (double)y1;

      if (yd != 0)
        y += (double)(i * yd) / r * ys;
      if (xd != 0)
        x += (double)(i * xd) / r * xs;

      for (size_t yy = c->height * 4; yy >= (size_t)y; --yy)
        CanvasSet (c, x, yy, color);
    }
}
void
CanvasDrawRect (Canvas *c, double x1_, double y1_, double x2_,
                double y2_, short color)
{
  const size_t x1 = NORMALIZE (x1_);
  const size_t y1 = NORMALIZE (y1_);
  const size_t x2 = NORMALIZE (x2_);
  const size_t y2 = NORMALIZE (y2_);

  const size_t xs = CANVAS_MAX (CANVAS_MIN (x1, x2), 0);
  const size_t xe = CANVAS_MIN (CANVAS_MAX (x1, x2), c->width*2);
  const size_t ys = CANVAS_MAX (CANVAS_MIN (y1, y2), 0);
  const size_t ye = CANVAS_MIN (CANVAS_MAX (y1, y2), c->height*4);

  for (size_t y = ys; y <= ye; ++y)
    for (size_t x = xs; x <= xe; ++x)
      CanvasSet (c, x, y, color);
}

#include "canvas.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Apparently this isn't included by <ncurses.h> */
extern int waddnwstr(WINDOW *win, const wchar_t *wstr, int n);

#define NORMALIZE(x) (size_t) round((double)(x))

static const uint16_t pixel_map[4][2]
    = {{0x01, 0x08}, {0x02, 0x10}, {0x04, 0x20}, {0x40, 0x80}};

static const int braille_char_offset = 0x2800;

Canvas *
CanvasCreate(WINDOW *win) {
    Canvas *c = (Canvas *)malloc(sizeof(Canvas));
    c->chars = NULL;
    c->colors = NULL;
    CanvasResize(c, win);
    return c;
}

Canvas *
CanvasCreateSized(size_t width, size_t height) {
    Canvas *c = (Canvas *)malloc(sizeof(Canvas));
    c->chars = NULL;
    c->colors = NULL;
    CanvasResizeTo(c, width, height);
    return c;
}

void
CanvasDelete(Canvas *c) {
    free(c->chars);
    free(c->colors);
    free(c);
}

void
CanvasClear(Canvas *c) {
    for (size_t i = 0; i < c->width * c->height; ++i) {
        c->chars[i] = 0;
        c->colors[i] = 0;
    }
}

void
CanvasResize(Canvas *c, WINDOW *win) {
    CanvasResizeTo(c, getmaxx(win) - 2, getmaxy(win) - 2);
}

void
CanvasResizeTo(Canvas *c, size_t width, size_t height) {
    /* No need to preserve old content. */
    free(c->chars);
    free(c->colors);
    c->chars = calloc(width * height, sizeof(uint8_t));
    c->colors = malloc(width * height * sizeof(short));
    for (size_t i = 0; i < width * height; ++i) {
        c->colors[i] = 0;
    }
    c->width = width;
    c->height = height;
}

void
CanvasSet(Canvas *c, double x_, double y_, short color) {
    const int x = NORMALIZE(x_);
    const int y = NORMALIZE(y_);
    const int col = x / 2;
    const int row = y / 4;
    if (col < 0 || col >= (int)c->width || row < 0 || row >= (int)c->height) {
        return;
    }
    const size_t idx = (x / 2) + (y / 4) * c->width;

    c->chars[idx] |= pixel_map[y % 4][x % 2];
    c->colors[idx] = color;
}

void
CanvasDraw(const Canvas *c, WINDOW *win) {
    CanvasDrawAt(c, win, 1, 1);
}

void
CanvasDrawAt(const Canvas *c, WINDOW *win, int x0, int y0) {
    size_t row_off;
    wchar_t ch[] = {0, 0};
    uint8_t b;

    for (size_t y = 0; y < c->height; ++y) {
        row_off = y * c->width;
        wmove(win, y0 + y, x0);
        for (size_t x = 0; x < c->width; ++x) {
            b = c->chars[row_off + x];
            wattron(win, COLOR_PAIR(c->colors[row_off + x]));
            if (b || 1) {
                ch[0] = braille_char_offset + c->chars[row_off + x];
                waddnwstr(win, ch, 1);
            } else {
                waddch(win, ' ');
            }
            wattroff(win, COLOR_PAIR(c->colors[row_off + x]));
        }
    }
}

void
CanvasDrawLine(
    Canvas *c, double x1_, double y1_, double x2_, double y2_, short color
) {
    const int x1 = NORMALIZE(x1_);
    const int y1 = NORMALIZE(y1_);
    const int x2 = NORMALIZE(x2_);
    const int y2 = NORMALIZE(y2_);

    const int xd = abs(x1 - x2);
    const int yd = abs(y1 - y2);
    const int xs = x1 <= x2 ? 1 : -1;
    const int ys = y1 <= y2 ? 1 : -1;

    const double r = xd > yd ? xd : yd;

    double x, y;
    for (size_t i = 0; i <= r; ++i) {
        x = (double)x1;
        y = (double)y1;

        if (xd != 0) {
            x += (double)(i * xd) / r * xs;
            // Note: this clipping is specialized for the graph drawing so we
            // only need check the X coordinate.
            if (x < 0) {
                continue;
            }
        }
        if (yd != 0) {
            y += (double)(i * yd) / r * ys;
        }

        CanvasSet(c, x, y, color);
    }
}

#define CANVAS_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CANVAS_MIN(a, b) (((a) < (b)) ? (a) : (b))

void
CanvasDrawLineFill(
    Canvas *c, double x1_, double y1_, double x2_, double y2_, short color
) {
    const int x1 = NORMALIZE(x1_);
    const int y1 = NORMALIZE(y1_);
    const int x2 = NORMALIZE(x2_);
    const int y2 = NORMALIZE(y2_);

    const int xd = abs(x1 - x2);
    const int yd = abs(y1 - y2);
    const int xs = x1 <= x2 ? 1 : -1;
    const int ys = y1 <= y2 ? 1 : -1;

    const double r = CANVAS_MAX(xd, yd);

    double x, y;
    for (size_t i = 0; i <= r; ++i) {
        x = (double)x1;
        y = (double)y1;

        if (xd != 0) {
            x += (double)(i * xd) / r * xs;
            if (x < 0) {
                continue;
            }
        }

        if (yd != 0) {
            y += (double)(i * yd) / r * ys;
        }

        for (size_t yy = c->height * 4; yy >= (size_t)y; --yy) {
            CanvasSet(c, x, yy, color);
        }
    }
}

void
CanvasDrawRect(
    Canvas *c, double x1_, double y1_, double x2_, double y2_, short color
) {
    // Like in `CanvasDrawLine` this is specific to the graph drawing where only
    // the the left x can be negative.
    if (x1_ < 0.0) {
        x1_ = 0.0;
    }
    const size_t x1 = NORMALIZE(x1_);
    const size_t y1 = NORMALIZE(y1_);
    const size_t x2 = NORMALIZE(x2_);
    const size_t y2 = NORMALIZE(y2_);

    const size_t xs = CANVAS_MAX(CANVAS_MIN(x1, x2), 0);
    const size_t xe = CANVAS_MIN(CANVAS_MAX(x1, x2), c->width * 2 - 1);
    const size_t ys = CANVAS_MAX(CANVAS_MIN(y1, y2), 0);
    const size_t ye = CANVAS_MIN(CANVAS_MAX(y1, y2), c->height * 4 - 1);

    for (size_t y = ys; y <= ye; ++y) {
        for (size_t x = xs; x <= xe; ++x) {
            CanvasSet(c, x, y, color);
        }
    }
}

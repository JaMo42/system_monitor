#pragma once
#include "stdafx.h"
#include "canvas/canvas.h"

#define MAX_SAMPLES(win, scale) ((getmaxx (win) - 2) / scale + 2)

/* Colors */
extern const short C_BORDER;
extern const short C_TITLE;
extern const short C_CPU_AVG;
extern const short C_CPU_GRAPHS[8];
extern const short C_MEM_MAIN;
extern const short C_MEM_SWAP;
extern const short C_NET_RECEIVE;
extern const short C_NET_TRANSMIT;
extern const short C_PROC_HEADER;
extern const short C_PROC_PROCESSES;
extern const short C_PROC_CURSOR;
extern const short C_PROC_HIGHLIGHT;
extern const short C_PROC_HIGH_PERCENT;
extern const short C_PROC_BRANCHES;
extern const short C_PROC_PATH;
extern const short C_PROC_COMMAND;
extern const short C_PROC_OPT;
extern const short C_PROC_ARG;
extern const short C_DISK_FREE;
extern const short C_DISK_USED;
extern const short C_DISK_ERROR;
extern const short C_BATTERY_FILL;

typedef struct {
  const char *unit;
  double size;
} Size_Format;

/* Draw a rectangle */
void DrawRect(WINDOW *win, int x, int y, int width, int height);

/* Draw a window border */
void Border (WINDOW *w);

/* Draw a window border and title:
   +-< title >-----+
   |               |
   +---------------+ */
void DrawWindow (WINDOW *w, const char *title);

/* Draw extra information onto the window border:
   +-< title >-----< info >-+
   |                        |
   +------------------------+ */
void DrawWindowInfo (WINDOW *w, const char *info);

/* Draw extra information onto bottom window border:
   +-< title >-----+
   |               |
   +------< info >-+ */
void DrawWindowInfo2 (WINDOW *w, const char *info);

Size_Format GetSizeFormat (size_t size);

void FormatSize (WINDOW *win, size_t size, bool pad);

void PrintN (WINDOW *win, int ch, unsigned n);

char* StringPush (char *buf, const char *s);

void PushStyle (WINDOW *win, attr_t attributes, short color);

void PopStyle (WINDOW *win);

/** Prints a padded percentage without adding unnecessary spaces.
    The given coordinates are the leftmost cell and the printed text is at most
    4 characters wide ("100%"). */
void PrintPercentage(WINDOW *win, int x, int y, double p);

// asprintf is not declared for me, even with _GNU_SOURCE...
char *Format(const char *fmt, ...);

/** Reads up to 31 characters from a file.  The returned pointer points to an
    internal buffer.  If trim_end is true the last character is replaced by
    the null terminator instead of placing it one after the last character. */
char *ReadSmallFile(const char *pathname, bool trim_end);

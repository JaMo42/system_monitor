#pragma once
#include "stdafx.h"
#include "canvas/canvas.h"

#define MAX_SAMPLES(win, scale) ((getmaxx (win) - 2) / scale + 2)

/* Colors */
extern const short C_BORDER;
extern const short C_TITLE;
extern const short C_ACCENT;
extern const short C_GRAPH_TABLE[8];
extern const short C_MEM_MAIN;
extern const short C_MEM_SWAP;
extern const short C_NET_RECIEVE;
extern const short C_NET_TRANSMIT;

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

void FormatSize (WINDOW *win, size_t size, bool pad);


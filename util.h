#pragma once
#include "stdafx.h"
#include "canvas/canvas.h"

/* Colors */
extern const short C_BORDER;
extern const short C_TITLE;
extern const short C_ACCENT;
extern const short C_GRAPH_TABLE[8];
extern const short C_MEM_MAIN;
extern const short C_MEM_SWAP;

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

void FormatSize (size_t size, WINDOW *win);

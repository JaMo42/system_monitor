#pragma once
#include "stdafx.h"

extern const short C_BORDER;
extern const short C_TITLE;
extern const short C_ACCENT;
extern const short C_GRAPH_TABLE[8];

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

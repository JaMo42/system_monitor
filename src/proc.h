#pragma once
#include "stdafx.h"
#include "widget.h"

#define PROC_SORT_CPU "-pcpu"
#define PROC_SORT_MEM "-pmem"
#define PROC_SORT_PID "pid"  // low -> high
#define PROC_SORT_INVPID "-pid"  // high -> low

void ProcInit (WINDOW *win, unsigned graph_scale);
void ProcQuit ();
void ProcUpdate ();
void ProcDraw (WINDOW *win);
void ProcResize (WINDOW *win);
void ProcMinSize (int *width_return, int *height_return);
bool ProcHandleInput (int key);
void ProcDrawBorder (WINDOW *win);

void ProcCursorUp ();
void ProcCursorDown ();
void ProcCursorPageUp ();
void ProcCursorPageDown ();
void ProcCursorTop ();
void ProcCursorBottom ();
void ProcSetSort (const char *mode);
void ProcToggleTree ();

void ProcBeginSearch ();
void ProcSearchNext ();
void ProcSearchPrev ();

extern Widget proc_widget;

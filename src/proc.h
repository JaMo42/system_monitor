#pragma once
#include "stdafx.h"
#include "widget.h"

// Initial forest mode setting
extern bool proc_forest;
// Initial kernel thread visibility
extern bool proc_kthreads;

extern Widget proc_widget;

void ProcInit (WINDOW *win);
void ProcQuit ();
void ProcUpdate ();
void ProcDraw (WINDOW *win);
void ProcResize (WINDOW *win);
void ProcMinSize (int *width_return, int *height_return);
bool ProcHandleInput (int key);
void ProcHandleMouse (Mouse_Event *event);
void ProcDrawBorder (WINDOW *win);

void ProcCursorUp ();
void ProcCursorDown ();
void ProcCursorPageUp ();
void ProcCursorPageDown ();
void ProcCursorTop ();
void ProcCursorBottom ();
void ProcSetSort (const char *mode);

void ProcBeginSearch ();
void ProcSearchNext ();
void ProcSearchPrev ();


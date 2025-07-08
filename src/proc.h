#pragma once
#include "stdafx.h"
#include "widget.h"

// Initial forest mode setting
extern bool proc_forest;
// Initial kernel thread visibility
extern bool proc_kthreads;
extern bool proc_command_only;
extern unsigned proc_scroll_speed;

extern Widget proc_widget;

void ProcInit(WINDOW *win);
void ProcQuit();
void ProcUpdate();
void ProcDraw(WINDOW *win);
void ProcResize(WINDOW *win);
void ProcMinSize(int *width_return, int *height_return);
bool ProcHandleInput(int key);
void ProcHandleMouse(Mouse_Event *event);
void ProcDrawBorder(WINDOW *win);

void ProcCursorUp(unsigned count);
void ProcCursorDown(unsigned count);
void ProcCursorPageUp();
void ProcCursorPageDown();
void ProcCursorTop();
void ProcCursorBottom();
void ProcSetSort(const char *mode);

void ProcBeginSearch();
void ProcSearchNext();
void ProcSearchPrev();

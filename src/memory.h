#pragma once
#include "stdafx.h"
#include "widget.h"

void MemoryInit (WINDOW *win, unsigned graph_scale);
void MemoryQuit ();
void MemoryUpdate ();
void MemoryDraw (WINDOW *win);
void MemoryResize (WINDOW *win);
void MemoryMinSize (int *width_return, int *height_return);

extern Widget mem_widget;


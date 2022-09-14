#pragma once
#include "stdafx.h"
#include "widget.h"

extern bool cpu_show_avg;
extern bool cpu_scale_height;

extern Widget cpu_widget;

void CpuInit (WINDOW *win, unsigned graph_scale);
void CpuQuit ();
void CpuUpdate ();
void CpuDraw (WINDOW *win);
void CpuResize (WINDOW *win);
void CpuMinSize (int *width_return, int *height_return);
bool CpuHandleInput (int key);
void CpuDrawBorder (WINDOW *win);

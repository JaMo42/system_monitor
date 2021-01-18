#pragma once
#include "stdafx.h"
#include "widget.h"

extern bool cpu_show_avg;

void CpuInit (WINDOW *win, unsigned graph_scale);
void CpuQuit ();
void CpuUpdate ();
void CpuDraw (WINDOW *win);
void CpuResize (WINDOW *win);

extern Widget cpu_widget;


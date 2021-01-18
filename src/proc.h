#pragma once
#include "stdafx.h"
#include "widget.h"

void ProcInit (WINDOW *win, unsigned graph_scale);
void ProcQuit ();
void ProcUpdate ();
void ProcDraw (WINDOW *win);
void ProcResize (WINDOW *win);

extern Widget proc_widget;


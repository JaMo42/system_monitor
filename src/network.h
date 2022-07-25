#pragma once
#include "stdafx.h"
#include "widget.h"

void NetworkInit (WINDOW *win, unsigned graph_scale);
void NetworkQuit ();
void NetworkUpdate ();
void NetworkDraw (WINDOW *win);
void NetworkResize (WINDOW *win);
void NetworkMinSize (int *width_return, int *height_return);

extern Widget net_widget;


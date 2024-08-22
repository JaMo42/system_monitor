#pragma once
#include "stdafx.h"
#include "widget.h"

void NetworkInit(WINDOW *win);
void NetworkQuit();
void NetworkUpdate();
void NetworkDraw(WINDOW *win);
void NetworkResize(WINDOW *win);
void NetworkMinSize(int *width_return, int *height_return);
void NetworkDrawBorder(WINDOW *win);

extern Widget net_widget;
extern bool net_auto_scale;

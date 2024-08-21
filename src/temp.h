#pragma once
#include "stdafx.h"
#include "widget.h"

extern const char *temp_filter;
extern bool temp_show_average;

extern Widget temp_widget;

void TempInit(WINDOW * win);
void TempQuit();
void TempUpdate();
void TempDraw(WINDOW * win);
void TempResize(WINDOW * win);
void TempMinSize(int *width_return, int *height_return);
void TempDrawBorder(WINDOW * win);

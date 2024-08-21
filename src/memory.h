#pragma once
#include "stdafx.h"
#include "widget.h"

extern Widget mem_widget;

void MemoryInit(WINDOW * win);
void MemoryQuit();
void MemoryUpdate();
void MemoryDraw(WINDOW * win);
void MemoryResize(WINDOW * win);
void MemoryMinSize(int *width_return, int *height_return);
void MemoryDrawBorder(WINDOW * win);

unsigned long MemoryTotal();

#pragma once
#include "stdafx.h"
#include "widget.h"

extern bool disk_vertical;
extern const char *disk_fs;

void DiskInit (WINDOW *win);
void DiskQuit ();
void DiskUpdate ();
void DiskDraw (WINDOW *win);
void DiskResize (WINDOW *win);
void DiskMinSize (int *width_return, int *height_return);
void DiskPreferredSize (int *width_return, int *height_return);
void DiskDrawBorder (WINDOW *win);

extern Widget disk_widget;

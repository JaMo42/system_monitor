#pragma once
#include "stdafx.h"
#include "ui.h"
#include "widget.h"

extern Layout *ui;

extern Widget *all_widgets[];

extern Widget *bottom_right_widget;

extern pthread_mutex_t draw_mutex;

extern void *overlay_data;

extern void (*DrawOverlay)(void *);

extern bool (*HandleInput)(int);

bool MainHandleInput(int key);

void HandleResize();

void DrawHelpInfo();

#pragma once
#include "stdafx.h"
#include "widget.h"
#include "ui.h"

extern Layout *ui;

extern struct Widget *all_widgets[];

extern pthread_mutex_t draw_mutex;

extern void *overlay_data;

extern void (*DrawOverlay) (void *);

extern bool (*HandleInput) (int);

bool MainHandleInput (int key);

void HandleResize ();

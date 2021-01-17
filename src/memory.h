#pragma once
#include "stdafx.h"
#include "canvas/canvas.h"

extern List *mem_main_usage;
extern List *mem_swap_usage;
extern unsigned long mem_main_total;
extern unsigned long mem_swap_total;
extern size_t mem_max_samples;
extern size_t mem_samples;

extern Canvas *mem_canvas;
extern int mem_graph_scale;

void MemoryInit (size_t max_samples);
void MemoryQuit ();
void MemorySetMaxSamples (size_t s);
void MemoryUpdate ();
void MemoryDraw (WINDOW *win);

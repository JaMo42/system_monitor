#pragma once
#include "stdafx.h"
#include "canvas/canvas.h"

extern List **cpu_usages;
extern int cpu_count;
extern size_t cpu_max_samples;
extern size_t cpu_samples;

extern bool cpu_show_avg;
extern Canvas *cpu_canvas;
extern int cpu_graph_scale;

void CpuInit (size_t max_samples);
void CpuQuit ();
void CpuSetMaxSamples (size_t s);
void CpuUpdate ();
void CpuDraw (WINDOW *);

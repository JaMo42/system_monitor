#pragma once
#include "stdafx.h"
#include "canvas/canvas.h"

extern List *net_recieve;
extern List *net_transmit;
extern unsigned long net_recieve_total;
extern unsigned long net_transmit_total;
extern unsigned net_max_samples;
extern unsigned net_samples;

extern Canvas *net_canvas;
extern int net_graph_scale;

void NetworkInit (unsigned max_samples);
void NetworkQuit ();
void NetworkSetMaxSamples (unsigned s);
void NetworkUpdate ();
void NetworkDraw (WINDOW *);


#pragma once
#include "../stdafx.h"
#include "proc_map.h"
#include "jiffy_list.h"
#include "ps.h"

#define INVALID_GENERATION 0

extern unsigned page_shift_amount;
extern Proc_Map procs;
extern uint8_t current_generation;
extern Jiffy_List cpu_times;
extern unsigned long mem_total;
extern bool forest;
extern int sorting_mode;
extern VECTOR(Proc_Data*) sorted_procs;
extern bool show_kthreads;


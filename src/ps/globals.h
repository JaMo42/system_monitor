#pragma once
#include "../stdafx.h"
#include "jiffy_list.h"
#include "proc_map.h"
#include "ps.h"

#define INVALID_GENERATION 0

/** Value to left-shift a page count with to get KB. */
extern unsigned page_shift_amount;
/** The internal process record.  This owns the process information, whereas
    `sorted_procs` only holds references to them. */
extern Proc_Map procs;
/** The current generation, to check whether a process in `procs` does no
    longer exist. */
extern uint8_t current_generation;
/** The total CPU times (work+idle) at each update. */
extern Jiffy_List cpu_times;
/** The total amount of memory, in KB. */
extern unsigned long mem_total;
/** The sorted and potentially forested process list. */
extern VECTOR(Proc_Data *) sorted_procs;
/** Whether to include kernel threads in the record. */
extern bool show_kthreads;
/** Sorting mode */
extern int sorting_mode;
/** Whether to arrange processes trees as such. */
extern bool forest;
/** Whether to sum up cpu and memory usage in trees. */
extern bool sum_children;

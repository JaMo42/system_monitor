#pragma once
#include "../stdafx.h"
#include "command_line.h"
#include "jiffy_list.h"

#define PS_MAX_LEVEL 8U

typedef struct Proc_Data {
  /*** Data ***/
    pid_t pid;
    pid_t parent;
    Jiffy_List cpu_times;
    unsigned long start_time;
    unsigned long memory;
    Command_Line command_line;
      VECTOR(struct Proc_Data *) children;
    unsigned long total_cpu_time;
    unsigned long total_memory;
  /*** Display info ***/
    // For forest mode, depth in the tree (may be larger than PS_MAX_LEVEL)
    unsigned tree_level;
    // What prefixes it should have in forest mode, these need to be set during
    // sorting as we want to be able to draw any range of processes and may
    // start drawing within a tree
    int8_t tree_prefix[PS_MAX_LEVEL];
    bool tree_folded;
  /*** Extra data ***/
    bool search_match;
} Proc_Data;

enum {
    PS_SORT_PID_DESCENDING,
    PS_SORT_PID_ASCENDING,
    PS_SORT_CPU_DESCENDING,
    PS_SORT_CPU_ASCENDING,
    PS_SORT_MEM_DESCENDING,
    PS_SORT_MEM_ASCENDING,
};

/**
 * Tree prefixes:
 *        foo
 *        |- bar
 *     1> |   |- bar1 <3
 *        |   '- bar2 <5
 *        '- baz
 *     2>     '- baz1
 *              ...
 *               |- at_max_level
 *                7> :  after_max_level
 *                   :  after_max_level
 *        bar
 *     4> |- ... folded
 *     6> '- ... folded
 *     8> ... baz
 *
 * 1: PS_PREFIX_OUTER_SIBLING
 * 2: PS_PREFIX_OUTER_NO_SIBLING
 * 3: PS_PREFIX_SIBLING
 * 4: PS_PREFIX_SIBLING_FOLDED
 * 5: PS_PREFIX_NO_SIBLING
 * 6: PS_PREFIX_NO_SIBLING_FOLDED
 * 7: PS_PREFIX_MAX_LEVEL
 * 8: PS_PREFIX_TOP_FOLDED
 *
 * Note: PS_PREFIX_MAX_LEVEL is never stored in the tree_prefix array,
 *       instead tree_level may be greater than PS_MAX_LEVEL in which case
 *       PS_PREFIX_MAX_LEVEL is an implied value for the excess elements.
 */
enum {
    /* PS_PREFIX_TOP is just an empty prefix, it's used so we can use the same
       logic for printing toplevel prefixes as we used for the other prefixes. */
    PS_PREFIX_TOP,
    PS_PREFIX_TOP_FOLDED,
    PS_PREFIX_OUTER_SIBLING,
    PS_PREFIX_OUTER_NO_SIBLING,
    PS_PREFIX_SIBLING,
    PS_PREFIX_SIBLING_FOLDED,
    PS_PREFIX_NO_SIBLING,
    PS_PREFIX_NO_SIBLING_FOLDED,
    PS_PREFIX_MAX_LEVEL,
};

/** Initializes the process snapshot library.  Must be called before any of the
    other functions. */
void ps_init();

/** Reads process information. */
void ps_update();

/** Sets thr sorting mode and sorts the processes accordingly, if the new mode
    is different from the last. */
void ps_set_sort(int mode);

/** Toggles sorting processes into trees and sorts the processes accordingly. */
void ps_toggle_forest();

/** Toggles inclusion of kernel threads.  This causes an update and a sort as
    kernel threads are completely removed from the internal record if disabled.
 */
void ps_toggle_kthreads();

/** Toggles summation of of cpu and memory usage in forest mode.  This also
    causes a sort. */
void ps_toggle_sum_children();

/** Releases all memory held by the library. */
void ps_quit();

/** Sorts the processes only if forest mode is enabled. */
void ps_remake_forest();

/** Returns a reference to the sorted process list. This is zero-cost. */
VECTOR(Proc_Data *)
ps_get_procs();

/** Gets the total recorded CPU time period.  This is the divisor to get
    percentage CPU usage, given a work period as dividend. */
 unsigned long ps_cpu_period();

/** Returns the total amount of main memory. */
 unsigned long ps_total_memory();

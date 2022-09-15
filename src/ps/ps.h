#pragma once
#include "../stdafx.h"
#include "jiffy_list.h"

#define PS_MAX_LEVEL 8U

typedef struct Proc_Data {
  /*** Data ***/
  pid_t pid;
  pid_t parent;
  Jiffy_List cpu_times;
  unsigned long start_time;
  unsigned long memory;
  char *command_line;
  VECTOR(struct Proc_Data *) children;
  /*** Display info ***/
  // For forest mode, depth in the tree (may be larger than PS_MAX_LEVEL)
  unsigned tree_level;
  // What prefixes it should have in forest mode, these need to be set during
  // sorting as we want to be able to draw any range of processes and may
  // start drawing within a tree
  int8_t tree_prefix[PS_MAX_LEVEL];
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
 *        |   '- bar2 <4
 *        '- baz
 *     2>     '- baz1
 *        ...
 *               |- at_max_level
 *            5> :  after_max_level
 *               :  after_max_level
 * 1: PS_PREFIX_OUTER_SIBLING
 * 2: PS_PREFIX_OUTER_NO_SIBLING
 * 3: PS_PREFIX_SIBLING
 * 4: PS_PREFIX_NO_SIBLING
 * 5: PS_PREFIX_MAX_LEVEL
 */
enum {
  PS_PREFIX_OUTER_SIBLING,
  PS_PREFIX_OUTER_NO_SIBLING,
  PS_PREFIX_SIBLING,
  PS_PREFIX_NO_SIBLING,
  PS_PREFIX_MAX_LEVEL
};

void ps_init ();

void ps_update ();

void ps_set_sort (int mode);

void ps_toggle_forest ();

void ps_quit ();

VECTOR(Proc_Data*) ps_get_procs ();

unsigned long ps_cpu_period ();

unsigned long ps_total_memory ();

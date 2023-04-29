#pragma once
#include "../stdafx.h"
#include "util.h"

typedef struct {
  pid_t pid;
  pid_t parent;
  unsigned long utime;
  unsigned long stime;
  unsigned long start_time;
  long resident_memory;
} Proc_Stat;

bool readstat (int dif_fd, Proc_Stat *stat_out);

/**
 * Read data required to update an existing program from the stat file.
 * Currently reads the utime, stime, and resident_memory fields.
 */
bool readstat_update (int dif_fd, Proc_Stat *stat_out);

unsigned long get_total_cpu ();

/** Returns total memory in KB */
unsigned long get_total_memory ();


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

char* parse_commandline (File_Content *cmdline);

unsigned long get_total_cpu ();

/** Returns total memory in KB */
unsigned long get_total_memory ();

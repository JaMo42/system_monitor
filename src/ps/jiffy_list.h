#pragma once
#include "../stdafx.h"

#define JIFFY_LIST_SIZE 5

/** Cirular buffer with a fixed size, storing cpu jiffies. */
typedef struct {
  unsigned long data[JIFFY_LIST_SIZE];
  unsigned begin;
} Jiffy_List;

/** Initializes the jiffy list, filling it with the initial sample */
void jiffy_list_construct (Jiffy_List *self, unsigned long initial_sample);

/** Pushes a new smaple the list, removing the oldest sample */
void jiffy_list_push (Jiffy_List *self, unsigned long sample);

/** Returns the jiffies between the first and last sample. */
unsigned long jiffy_list_period (Jiffy_List *self);

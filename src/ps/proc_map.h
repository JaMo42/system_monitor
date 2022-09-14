#pragma once
#include "../stdafx.h"
#include "ps.h"

/** Callback for allocate data. */
typedef Proc_Data* (*Proc_Map_Alloc) ();

/** Callback for deallocating data. */
typedef void (*Proc_Map_Free) (Proc_Data *);

/** Callback for removing data. */
typedef void (*Proc_Map_Remove) (Proc_Data *);

typedef enum Proc_Map_State Proc_Map_State;

typedef struct {
  pid_t key;
  Proc_Data *data;
  int8_t state;
  bool mark;
} Proc_Map_Bucket;

typedef struct {
  Proc_Map_Bucket *data;
  size_t capacity;
  size_t size;
  size_t tombs;
  Proc_Map_Alloc alloc_data;
  Proc_Map_Free free_data;
  Proc_Map_Remove remove_data;
} Proc_Map;

void proc_map_construct (Proc_Map *self, Proc_Map_Alloc alloc_data,
                         Proc_Map_Free free_data, Proc_Map_Remove remove_data);

void proc_map_destruct (Proc_Map *self);

Proc_Data* proc_map_insert_or_get (Proc_Map *self, pid_t pid, bool mark);

Proc_Data* proc_map_get (Proc_Map *self, pid_t pid);

void proc_map_erase_unmarked (Proc_Map *self, bool expected_mark);

Proc_Map_Bucket *proc_map_begin (Proc_Map *self);
Proc_Map_Bucket *proc_map_next (Proc_Map_Bucket *it);
#define proc_map_for_each(self_) \
  for (Proc_Map_Bucket *it = proc_map_begin (self_); it; it = proc_map_next (it))

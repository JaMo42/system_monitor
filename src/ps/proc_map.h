#pragma once
#include "../stdafx.h"
#include "ps.h"

/** Callback for allocate data. */
typedef Proc_Data *(*Proc_Map_Alloc)();

/** Callback for deallocating data. */
typedef void (*Proc_Map_Free)(Proc_Data *);

/** Callback for removing data. */
typedef void (*Proc_Map_Remove)(Proc_Data *);

typedef struct {
    Proc_Data *data;
    pid_t key;
    int8_t state;
    uint8_t generation;
} Proc_Map_Bucket;

/** A PID to process information map using generation numbers to check for
    outdated entries. */
typedef struct {
    Proc_Map_Bucket *data;
    size_t capacity;
    size_t size;
    size_t tombs;
    Proc_Map_Alloc alloc_data;
    Proc_Map_Free free_data;
    Proc_Map_Remove remove_data;
} Proc_Map;

/** Return value of `proc_map_insert_or_get` */
typedef struct {
  /** The process data inside the entry. */
    Proc_Data *value;
  /** The generation number of the entry. */
    uint8_t *generation;
  /** Whether the entry was newly created. */
    bool is_new;
} Proc_Map_Insert_Result;

/** Constructs a process map.  `alloc_data` is used to allocate uninitialized
    data for a bucket.  `free_data` is used to release memory of a bucket.
    `remove_data` is used for additional removal logic of a bucket and is called
    before `free_data` when removing a bucket. */
void proc_map_construct(Proc_Map *self, Proc_Map_Alloc alloc_data,
                        Proc_Map_Free free_data, Proc_Map_Remove remove_data);

/** Releases all memory held by the map.  This only calls `free_data` on buckets
    but not `remove_data`. */
void proc_map_destruct(Proc_Map *self);

/** Either gets an entry or creates it if it does not exist.  The returned
    structure holds references to the process data and generation of the entry
    as well as a flag whether it was newly created.  The generation should be
    updated by the caller as the map itself only holds the generation of entries
    but does not manage generations in any other way. */
Proc_Map_Insert_Result proc_map_insert_or_get(Proc_Map *self, pid_t pid);

/** Gets process data from the map.  If there is no process with the given ID
    `NULL` is returned. */
Proc_Data *proc_map_get(Proc_Map *self, pid_t pid);

/** Erases all entries that do not match the given generation number. */
void proc_map_erase_outdated(Proc_Map *self, uint8_t expected_generation);

/** Returns the first non-empty bucket. */
Proc_Map_Bucket *proc_map_begin(Proc_Map *self);
/** Returns the next non-empty bucket. */
Proc_Map_Bucket *proc_map_next(Proc_Map_Bucket *it);
#define proc_map_for_each(self_) \
  for (Proc_Map_Bucket *it = proc_map_begin(self_); it; it = proc_map_next(it))

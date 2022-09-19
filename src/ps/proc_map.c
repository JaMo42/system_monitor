#include "proc_map.h"

#define PROC_MAP_MAX_LOAD_FACTOR 75

#define INSERT_RESULT(bucket_, new_) \
  ((Proc_Map_Insert_Result) { \
    .value      = bucket->data, \
    .generation = &bucket->generation, \
    .is_new     = (new_) \
  })

enum {
  PROC_MAP_FREE,
  PROC_MAP_USED,
  PROC_MAP_TOMB
};

static bool is_prime (unsigned long long n) {
  if (n == 0 || n == 1) {
    return false;
  }
  if (n == 2 || n == 3) {
    return true;
  }
  else if (n%2 == 0 || n%3 == 0) {
    return false;
  }
  for (unsigned long long i = 5; i*i <= n; i += 6) {
    if (n%i == 0 || n%(i + 2) == 0) {
      return false;
    }
  }
  return true;
}

static size_t next_prime (size_t n)
{
  if (n == 2) {
    return 3ULL;
  } else {
    const int inc = is_prime (n) ? 2 : 1;
    n += inc;
    for (;!is_prime (n); n += inc) {
    }
    return n;
  }
}

static uint32_t double_hash (uint32_t hash) {
  // Robert Jenkins' 32 bit integer hash function
  hash = (hash + 0x7ed55d16) + (hash << 12);
  hash = (hash ^ 0xc761c23c) ^ (hash >> 19);
  hash = (hash + 0x165667b1) + (hash << 5);
  hash = (hash + 0xd3a2646c) ^ (hash << 9);
  hash = (hash + 0xfd7046c5) + (hash << 3);
  hash = (hash ^ 0xb55a4f09) ^ (hash >> 16);
  return hash;
}

static uint32_t hash_pid (pid_t pid) {
  return double_hash ((uint32_t)pid);
}

static Proc_Map_Bucket *proc_map_create (size_t count)
{
  Proc_Map_Bucket *d = calloc (count + 1, sizeof (Proc_Map_Bucket));
  // This is used as end marker in `proc_map_next`
  d[count].state = PROC_MAP_USED;
  d[count].data = NULL;
  return d;
}

void proc_map_construct (Proc_Map *self, Proc_Map_Alloc alloc_data,
                         Proc_Map_Free free_data, Proc_Map_Remove remove_data)
{
  const size_t initial_capacity = next_prime ((150*100 + PROC_MAP_MAX_LOAD_FACTOR)
                                              / PROC_MAP_MAX_LOAD_FACTOR);
  self->data = proc_map_create (initial_capacity);
  self->capacity = initial_capacity;
  self->size = 0;
  self->alloc_data = alloc_data;
  self->free_data = free_data;
  self->remove_data = remove_data;
}

void proc_map_destruct (Proc_Map *self)
{
  proc_map_for_each (self) {
    self->free_data (it->data);
  }
  free (self->data);
}

static Proc_Map_Bucket* proc_map_lookup_for_writing (Proc_Map *self, pid_t pid)
{
  uint32_t hash = hash_pid (pid);
  Proc_Map_Bucket *first_empty = NULL;
  for (;;) {
    Proc_Map_Bucket *bucket = &self->data[hash % self->capacity];
    if (bucket->state == PROC_MAP_USED) {
      if (bucket->key == pid) {
        return bucket;
      }
    } else {
      if (!first_empty) {
        first_empty = bucket;
      }
      if (bucket->state == PROC_MAP_FREE) {
        return first_empty;
      }
    }
    hash = double_hash (hash);
  }
}

static Proc_Map_Bucket* proc_map_lookup_existing (Proc_Map *self, pid_t pid)
{
  uint32_t hash = hash_pid (pid);
  for (;;) {
    Proc_Map_Bucket *bucket = &self->data[hash % self->capacity];
    if (bucket->state == PROC_MAP_USED && bucket->key == pid) {
      return bucket;
    } else if (bucket->state == PROC_MAP_FREE) {
      return NULL;
    }
    hash = double_hash (hash);
  }
}

static void proc_map_insert_during_rehash (Proc_Map *self, pid_t pid,
                                           uint8_t generation, Proc_Data *data)
{
  Proc_Map_Bucket *bucket = proc_map_lookup_for_writing (self, pid);
  bucket->key = pid;
  bucket->generation = generation;
  bucket->data = data;
  bucket->state = PROC_MAP_USED;
}

static void proc_map_rehash (Proc_Map *self, size_t capacity)
{
  capacity = next_prime (capacity);
  Proc_Map_Bucket *old_data = self->data;
  Proc_Map_Bucket *it = proc_map_begin (self);
  self->data = proc_map_create (capacity);
  self->capacity = capacity;
  self->tombs = 0;
  for (; it; it = proc_map_next (it)) {
    proc_map_insert_during_rehash (self, it->key, it->generation, it->data);
  }
  free (old_data);
}

static inline bool proc_map_should_rehash (Proc_Map *self)
{
  return (((self->size + self->tombs + 1) * 100)
          >= (self->capacity * PROC_MAP_MAX_LOAD_FACTOR));
}

Proc_Map_Insert_Result proc_map_insert_or_get (Proc_Map *self, pid_t pid)
{
  if (proc_map_should_rehash (self)) {
    proc_map_rehash (self, self->capacity << 1);
  }
  Proc_Map_Bucket *bucket = proc_map_lookup_for_writing (self, pid);
  if (bucket->state == PROC_MAP_USED) {
    return INSERT_RESULT (bucket, false);
  }
  self->tombs -= bucket->state == PROC_MAP_TOMB;
  bucket->key = pid;
  bucket->state = PROC_MAP_USED;
  bucket->data = self->alloc_data ();
  ++self->size;
  return INSERT_RESULT (bucket, true);
}

Proc_Data* proc_map_get (Proc_Map *self, pid_t pid)
{
  Proc_Map_Bucket *bucket;
  if ((bucket = proc_map_lookup_existing (self, pid))) {
    return bucket->data;
  }
  return NULL;
}

void proc_map_erase_outdated (Proc_Map *self, uint8_t expected_generation)
{
  proc_map_for_each (self) {
    if (it->state == PROC_MAP_USED && it->generation != expected_generation) {
      self->remove_data (it->data);
      self->free_data (it->data);
      it->state = PROC_MAP_TOMB;
      --self->size;
      ++self->tombs;
    }
  }
  if (self->tombs >= self->size && proc_map_should_rehash (self)) {
    proc_map_rehash (self, self->capacity);
  }
}

Proc_Map_Bucket *proc_map_begin (Proc_Map *self)
{
  if (self->data[0].state == PROC_MAP_USED) {
    return self->data + 0;
  } else {
    return proc_map_next (self->data);
  }
}

Proc_Map_Bucket *proc_map_next (Proc_Map_Bucket *it)
{
  ++it;
  while (it->state != PROC_MAP_USED)
    ++it;
  return it->data ? it : NULL;
}


#include "ps.h"
#include "globals.h"
#include "read_stat.h"
#include "proc_map.h"
#include "util.h"
#include "sort.h"

#define MAX_PID_LEN 7

static Proc_Data* add_or_update (pid_t pid, bool force);

static bool is_pid_dir (const struct dirent *entry)
{
  return entry->d_type == DT_DIR && isdigit (entry->d_name[0]);
}

static void update_process (int dir_fd, Proc_Data *process)
{
  Proc_Stat stat;
  readstat_update (dir_fd, &stat);
  jiffy_list_push (&process->cpu_times, stat.utime + stat.stime);
  process->memory = stat.resident_memory;
}

static void new_process (int dir_fd, Proc_Data *process, pid_t pid,
                         char *command_line)
{
  Proc_Stat stat;
  readstat (dir_fd, &stat);
  process->pid = pid;
  process->parent = stat.parent;
  jiffy_list_construct (&process->cpu_times, stat.utime + stat.stime);
  process->start_time = stat.start_time;
  process->memory = stat.resident_memory;
  process->command_line = command_line;
  process->tree_level = 0;
  process->search_match = false;

  if (stat.parent) {
    Proc_Data *parent = add_or_update (stat.parent, true);
    if (parent) {
      vector_for_each (parent->children, it) {
        if ((*it)->pid == pid) {
          return;
        }
      }
      vector_push (parent->children, process);
    } else {
      process->parent = 0;
    }
  }
}

static Proc_Data* add_or_update (pid_t pid, bool force)
{
  char pathname[6+MAX_PID_LEN+1] = "/proc/";
  snprintf (pathname+6, MAX_PID_LEN+1, "%d", pid);
  int dir_fd = open (pathname, O_RDONLY|O_DIRECTORY);
  Proc_Map_Insert_Result insert_result;
  File_Content cmdline = read_entire_file (dir_fd, "cmdline");
  char *command_line = NULL;
  if (cmdline.size <= 0) {
    if (force || show_kthreads) {
      command_line = get_comm (dir_fd);
    } else {
      close (dir_fd);
      return NULL;
    }
  }
  insert_result = proc_map_insert_or_get (&procs, pid);
  const uint8_t bucket_generation = *insert_result.generation;
  *insert_result.generation = current_generation;
  if (insert_result.is_new) {
    if (!command_line) {
      command_line = parse_commandline (&cmdline);
    }
    new_process (dir_fd, insert_result.value, pid, command_line);
  } else if (bucket_generation != current_generation) {
    update_process (dir_fd, insert_result.value);
  }
  close (dir_fd);
  return insert_result.value;
}

static void update_procs ()
{
  struct dirent *entry;
  DIR *proc_dir = opendir ("/proc");
  char *name;
  while ((entry = readdir (proc_dir))) {
    if (is_pid_dir (entry)) {
      name = entry->d_name;
      const pid_t pid = str2u (&name);
      add_or_update (pid, false);
    }
  }
  closedir (proc_dir);
}

static void unparent_children_of (Proc_Data *d)
{
  vector_for_each (d->children, it) {
    (*it)->parent = 0;
  }
}

static Proc_Data* proc_data_alloc ()
{
  Proc_Data *d = malloc (sizeof (Proc_Data));
  d->pid = 0;
  d->parent = 0;
  d->command_line = NULL;
  d->children = NULL;
  return d;
}

static void proc_data_free (Proc_Data *d)
{
  vector_free (d->children);
  free (d->command_line);
  free (d);
}

static void remove_child (Proc_Data *parent, pid_t child)
{
  const int end = (int)vector_size (parent->children);
  for (int i = 0; i != end; ++i)
    {
      if (parent->children[i]->pid == child)
        {
          vector_remove (parent->children, i);
          return;
        }
    }
}

static void proc_data_remove (Proc_Data *d)
{
  unparent_children_of (d);
  Proc_Data *p;
  if (d->parent && (p = proc_map_get (&procs, d->parent)))
    remove_child (p, d->pid);
}

void ps_init ()
{
  // For memory info in /proc/[pid]/stat, need to translate from pages to KB.
  const unsigned page_to_bytes_shift = __builtin_ctz((unsigned)sysconf (_SC_PAGESIZE));
  page_shift_amount = page_to_bytes_shift - 10;
  proc_map_construct (&procs, proc_data_alloc, proc_data_free,
                      proc_data_remove);
  jiffy_list_construct (&cpu_times, get_total_cpu ());
  current_generation = 1;
  mem_total = get_total_memory ();
  forest = false;
  show_kthreads = false;
  sum_children = true;
  sorting_mode = PS_SORT_CPU_DESCENDING;
  sorted_procs = vector_create (Proc_Data*, 50);
  ps_update ();
}

static VECTOR(Proc_Data*) ps_get_toplevel ()
{
  VECTOR(Proc_Data*) toplevel = vector_create (Proc_Data*, 16);
  proc_map_for_each (&procs) {
    if (it->data->parent == 0) {
      vector_push (toplevel, it->data);
    }
  }
  return toplevel;
}

static void ps_sum_proc_data (Proc_Data *proc)
{
  proc->total_cpu_time = jiffy_list_period (&proc->cpu_times);
  proc->total_memory = proc->memory;
  vector_for_each (proc->children, it) {
    Proc_Data *const p = *it;
    ps_sum_proc_data (p);
    proc->total_cpu_time += p->total_cpu_time;
    proc->total_memory += p->total_memory;
  }
}

static void ps_sum_child_data (VECTOR(Proc_Data *) toplevel)
{
  vector_for_each (toplevel, it) {
    ps_sum_proc_data (*it);
  }
}

static void ps_sort_procs_forest (VECTOR(Proc_Data*) procs, unsigned level,
                                  int8_t *prefix)
{
  sort_procs (procs, sorting_mode);
  vector_for_each (procs, it) {
    (*it)->tree_level = level;
    if (level) {
      if (level <= PS_MAX_LEVEL) {
        if (it == vector_end (procs) - 1) {
          prefix[level - 1] = PS_PREFIX_NO_SIBLING;
        } else {
          prefix[level - 1] = PS_PREFIX_SIBLING;
        }
      }
      memcpy ((*it)->tree_prefix, prefix, level);
    }
    vector_push (sorted_procs, (*it));
    if (!vector_empty ((*it)->children)) {
      if (level && level <= PS_MAX_LEVEL) {
        if (it == vector_end (procs) - 1) {
          prefix[level - 1] = PS_PREFIX_OUTER_NO_SIBLING;
        } else {
          prefix[level - 1] = PS_PREFIX_OUTER_SIBLING;
        }
      }
      ps_sort_procs_forest ((*it)->children, level + 1, prefix);
    }
  }
}

static void ps_sort_procs ()
{
  vector_clear (sorted_procs);
  if (forest) {
    VECTOR(Proc_Data*) toplevel = ps_get_toplevel ();
    if (sum_children) {
      ps_sum_child_data (toplevel);
    } else {
      proc_map_for_each (&procs) {
        it->data->total_cpu_time = jiffy_list_period (&it->data->cpu_times);
        it->data->total_memory = it->data->memory;
      }
    }
    int8_t prefix[PS_MAX_LEVEL];
    ps_sort_procs_forest (toplevel, 0, prefix);
  } else {
    proc_map_for_each (&procs) {
      it->data->tree_level = 0;
      it->data->total_cpu_time = jiffy_list_period (&it->data->cpu_times);
      it->data->total_memory = it->data->memory;
      vector_push (sorted_procs, it->data);
    }
    sort_procs (sorted_procs, sorting_mode);
  }
}

void ps_update ()
{
  ++current_generation;
  if (current_generation == INVALID_GENERATION)
    ++current_generation;
  update_procs ();
  proc_map_erase_outdated (&procs, current_generation);
  jiffy_list_push (&cpu_times, get_total_cpu ());
  ps_sort_procs ();
}

void ps_set_sort (int mode)
{
  if (mode != sorting_mode) {
    sorting_mode = mode;
    ps_sort_procs ();
  }
}

void ps_toggle_forest ()
{
  forest = !forest;
  ps_sort_procs ();
}

void ps_toggle_kthreads ()
{
  show_kthreads = !show_kthreads;
  ps_update ();
  ps_sort_procs ();
}

void ps_toggle_sum_children ()
{
  sum_children = !sum_children;
  ps_sort_procs ();
}

void ps_quit ()
{
  vector_free (sorted_procs);
  proc_map_destruct (&procs);
}

VECTOR(Proc_Data*) ps_get_procs ()
{
  return sorted_procs;
}

unsigned long ps_cpu_period ()
{
  return jiffy_list_period (&cpu_times);
}

unsigned long ps_total_memory ()
{
  return mem_total;
}

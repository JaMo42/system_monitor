#include "ps.h"
#include "globals.h"
#include "read_stat.h"
#include "proc_map.h"
#include "util.h"
#include "sort.h"

#define MAX_PID_LEN 7

static bool is_pid_dir (const struct dirent *entry)
{
  return entry->d_type == DT_DIR && isdigit (entry->d_name[0]);
}

static void add_or_update (Proc_Stat *ps, File_Content *cmdline)
{
  Proc_Data *d = proc_map_insert_or_get (&procs, ps->pid, current_mark);
  if (d->pid) {
    jiffy_list_push (&d->cpu_times, ps->utime + ps->stime);
  } else {
    d->pid = ps->pid;
    d->parent = ps->parent;
    jiffy_list_construct (&d->cpu_times, ps->utime + ps->stime);
    d->start_time = ps->start_time;
    d->memory = ps->resident_memory;
    d->command_line = parse_commandline (cmdline);
    d->children = NULL;
    d->tree_level = 0;
    d->search_match = false;

    if (d->parent) {
      Proc_Data *parent = proc_map_get (&procs, d->parent);
      if (parent) {
        vector_for_each (parent->children, it) {
          if ((*it)->pid == d->pid) {
            break;
          }
        }
        vector_push (parent->children, d);
      } else {
        d->parent = 0;
      }
    }
  }
}

static void update_procs ()
{
  struct dirent *entry;
  DIR *proc_dir = opendir ("/proc");
  int proc_dir_fd;
  char *name;
  Proc_Stat stat_data;
  char filename[6+MAX_PID_LEN+1] = "/proc/";
  File_Content cmdline;
  while ((entry = readdir (proc_dir))) {
    if (is_pid_dir (entry)) {
      name = entry->d_name;
      strncpy (filename+6, name, MAX_PID_LEN+1);
      proc_dir_fd = open (filename, O_RDONLY|O_DIRECTORY);
      if (readstat (proc_dir_fd, &stat_data)) {
        stat_data.pid = str2u (&name);
        cmdline = read_entire_file (proc_dir_fd, "cmdline");
        if (cmdline.size > 0) {
          add_or_update (&stat_data, &cmdline);
        }
      }
      close (proc_dir_fd);
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
  d->command_line = NULL;
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

static void remove_missing_parents ()
{
  proc_map_for_each (&procs) {
    if (it->data->pid == 0) {
      it->mark = !current_mark;
      unparent_children_of (it->data);
    }
  }
}

void ps_init ()
{
  // For memory info in /proc/[pid]/stat, need to translate from pages to KB.
  const unsigned page_to_bytes_shift = __builtin_ctz((unsigned)sysconf (_SC_PAGESIZE));
  page_shift_amount = page_to_bytes_shift - 10;
  proc_map_construct (&procs, proc_data_alloc, proc_data_free,
                      proc_data_remove);
  jiffy_list_construct (&cpu_times, get_total_cpu ());
  current_mark = true;
  mem_total = get_total_memory ();
  forest = false;
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
    int8_t prefix[PS_MAX_LEVEL];
    ps_sort_procs_forest (toplevel, 0, prefix);
  } else {
    proc_map_for_each (&procs) {
      it->data->tree_level = 0;
      vector_push (sorted_procs, it->data);
    }
    sort_procs (sorted_procs, sorting_mode);
  }
}

void ps_update ()
{
  current_mark = !current_mark;
  update_procs ();
  remove_missing_parents ();
  proc_map_erase_unmarked (&procs, current_mark);
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

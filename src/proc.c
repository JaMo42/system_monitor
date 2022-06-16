#include "proc.h"
#include "util.h"

#define PROC_MAX_COUNT 512

#define PROC_COMMAND_STORAGE_SIZE 128

Widget proc_widget = WIDGET(Proc);

extern struct timespec interval;
static unsigned long proc_time_passed;
static struct Process
{
  pid_t pid;
  char cmd[PROC_COMMAND_STORAGE_SIZE];
  double cpu;
  double mem;
} proc_processes[PROC_MAX_COUNT];
static size_t proc_count;
static const char *proc_sort = PROC_SORT_CPU;
static unsigned proc_cursor;
static pid_t proc_cursor_pid;
static pthread_mutex_t proc_data_mutex;
static unsigned proc_page_move_amount;
static bool proc_ps_tree = false;

static void ProcUpdateProcesses ();

void
ProcInit (WINDOW *win, unsigned graph_scale) {
  (void)graph_scale;
  proc_time_passed = 0;
  DrawWindow (win, "Processes");
  pthread_mutex_init (&proc_data_mutex, NULL);
  ProcUpdateProcesses ();
  proc_cursor_pid = proc_processes[0].pid;
  proc_page_move_amount = (getmaxy (win) - 3) / 2;
}

void
ProcQuit ()
{
  pthread_mutex_destroy (&proc_data_mutex);
}

static void
ProcUpdateProcesses ()
{
  char ps_cmd[128];
  sprintf (ps_cmd, "ps axo 'pid:10,pcpu:5,pmem:5,command:1,command:64' --sort=%s %s",
           proc_sort, proc_ps_tree ? "--forest" : "");
  FILE *ps = popen (ps_cmd, "r");
  char line[256];
  pid_t pid;
  int cpu_i, mem_i;
  char cpu_d, mem_d;
  double cpu, mem;

  proc_count = 0;

  (void)fgets (line, 256, ps);  /* Skip header */
  while (fgets (line, 256, ps))
    {
      if (line[23] == '[')
        continue;
      sscanf (line, "%d %d.%c %d.%c", &pid, &cpu_i, &cpu_d, &mem_i, &mem_d);
      cpu = cpu_i + (double)(cpu_d - '0') / 10.0;
      mem = mem_i + (double)(mem_d - '0') / 10.0;
      proc_processes[proc_count].pid = pid;
      strncpy (proc_processes[proc_count].cmd, line + 25, PROC_COMMAND_STORAGE_SIZE);
      proc_processes[proc_count].cmd[strlen (proc_processes[proc_count].cmd) - 1] = '\0';
      proc_processes[proc_count].cpu = cpu;
      proc_processes[proc_count].mem = mem;
      if (pid == proc_cursor_pid)
        proc_cursor = proc_count;

      ++proc_count;
      if (proc_count == PROC_MAX_COUNT)
        break;
    }

  pclose (ps);
}

void
ProcUpdate () {
  proc_time_passed += interval.tv_sec * 1000 + interval.tv_nsec / 1000000;
  if (proc_time_passed >= 2000)
    {
      proc_time_passed = 0;
      pthread_mutex_lock (&proc_data_mutex);
      ProcUpdateProcesses ();
      pthread_mutex_unlock (&proc_data_mutex);
    }
}

void
ProcDraw (WINDOW *win)
{
  pthread_mutex_lock (&proc_data_mutex);
  const unsigned rows = (unsigned)getmaxy (win) - 3;
  const unsigned rows_2 = rows / 2;
  const unsigned cpu_mem_off = getmaxx (win) - 13;
  unsigned first, last;

  if (proc_count <= rows)
    {
      first = 0;
      last = proc_count - 1;
    }
  else if (proc_cursor < rows_2)
    {
      first = 0;
      last = rows - 1;
    }
  else if (proc_cursor >= (proc_count - rows_2))
    {
      first = proc_count - rows;
      last = proc_count - 1;
    }
  else
    {
      first = proc_cursor - rows_2;
      last = proc_cursor + (rows - rows_2) - 1;
    }

  /* Window */
  char info[32];
  DrawWindow (win, "Processes");
  snprintf (info, 32, "%d - %d of %zu", first, last, proc_count);
  DrawWindowInfo (win, info);
  DrawWindowInfo2 (win, "Press ? for help");

  /* Header */
  wattron (win, A_BOLD | COLOR_PAIR (C_PROC_HEADER));
  wmove (win, 1, 1);
  waddstr (win, " PID      Command");
  wmove (win, 1, cpu_mem_off);
  waddstr (win, " CPU%  Mem%");
  wattroff (win, A_BOLD | COLOR_PAIR (C_PROC_HEADER));

  /* Processes */
  unsigned row = 2;
  for (; first <= last; ++first, ++row)
    {
      if (unlikely (first == proc_cursor))
        wattron (win, COLOR_PAIR (C_PROC_CURSOR));
      else
        wattron (win, COLOR_PAIR (C_PROC_PROCESSES));
      /* Content */
      wmove (win, row, 1);
      PrintN (win, ' ', getmaxx (win) - 2);
      wmove (win, row, 2);
      wprintw (win, "%-7d  %.*s",
               proc_processes[first].pid,
               cpu_mem_off - 10, proc_processes[first].cmd);
      wmove (win, row, cpu_mem_off);
      wprintw (win, "%5.1f %5.1f",
               proc_processes[first].cpu, proc_processes[first].mem);
      /*====*/
      if (unlikely (first == proc_cursor))
        wattroff (win, COLOR_PAIR (C_PROC_CURSOR));
      else
        wattroff (win, COLOR_PAIR (C_PROC_PROCESSES));
    }
  pthread_mutex_unlock (&proc_data_mutex);
}

void
ProcResize (WINDOW *win) {
  wclear (win);
  DrawWindow (win, "Processes");
  proc_page_move_amount = (getmaxy (win) - 3) / 2;
}

void
ProcCursorUp ()
{
  if (proc_cursor)
    --proc_cursor;
  proc_cursor_pid = proc_processes[proc_cursor].pid;
}

void
ProcCursorDown ()
{
  if ((proc_cursor + 1) < proc_count)
    ++proc_cursor;
  proc_cursor_pid = proc_processes[proc_cursor].pid;
}

void
ProcCursorPageUp ()
{
  if (proc_cursor >= proc_page_move_amount)
    proc_cursor -= proc_page_move_amount;
  else
    proc_cursor = 0;
  proc_cursor_pid = proc_processes[proc_cursor].pid;
}

void
ProcCursorPageDown ()
{
  if ((proc_cursor + proc_page_move_amount) < proc_count)
    proc_cursor += proc_page_move_amount;
  else
    proc_cursor = proc_count - 1;
  proc_cursor_pid = proc_processes[proc_cursor].pid;
}

void
ProcCursorTop ()
{
  proc_cursor = 0;
  proc_cursor_pid = proc_processes[0].pid;
}

void
ProcCursorBottom ()
{
  proc_cursor = proc_count - 1;
  proc_cursor_pid = proc_processes[proc_cursor].pid;
}

void
ProcSetSort (const char *mode)
{
  proc_sort = mode;
  proc_cursor_pid = 0;
  proc_cursor = 0;
  proc_time_passed = 2000;
}

void
ProcToggleTree ()
{
  proc_ps_tree = !proc_ps_tree;
  proc_cursor_pid = 0;
  proc_cursor = 0;
  proc_time_passed = 2000;
}


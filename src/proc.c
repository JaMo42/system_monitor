#include "proc.h"
#include "util.h"

#define PROC_MAX_COUNT 512

Widget proc_widget = WIDGET(Proc);


extern struct timespec interval;
static unsigned long proc_time_passed;
static struct Process
{
  pid_t pid;
  char cmd[64];
  double cpu;
  double mem;
} proc_processes[PROC_MAX_COUNT];
static size_t proc_count;
static const char *proc_sort = PROC_SORT_CPU;
static unsigned proc_cursor;
static pid_t proc_cursor_pid;

static void ProcUpdateProcesses ();

void
ProcInit (WINDOW *win, unsigned graph_scale) {
  (void)graph_scale;
  proc_time_passed = 0;
  DrawWindow (win, "Processes");
  ProcUpdateProcesses ();
  proc_cursor_pid = proc_processes[0].pid;
}

void
ProcQuit ()
{}

static void
ProcUpdateProcesses ()
{
  char ps_cmd[128] = "ps axo 'pid:10,comm:64,pcpu:5,pmem:5' --sort=";
  strcat (ps_cmd, proc_sort);
  FILE *ps = popen (ps_cmd, "r");
  char line[256];
  pid_t pid;
  char cmd[64];
  int cpu_i;
  int mem_i;
  char cpu_d;
  char mem_d;
  double cpu, mem;
  int len;

  proc_count = 0;

  (void)fgets (line, 256, ps);  /* Skip header */
  while (fgets (line, 256, ps))
    {
      /* apparently `%lf' doesn't parse deciamls and the memory is always 0 */
      sscanf (line, "%d %s %d.%c %d.%c", &pid, cmd, &cpu_i, &cpu_d, &mem_i, &mem_d);
      cpu = cpu_i + (double)(cpu_d - '0') / 10.0;
      mem = mem_i + (double)(mem_d - '0') / 10.0;
      len = strlen (cmd);
      proc_processes[proc_count].pid = pid;
      memcpy (proc_processes[proc_count].cmd, cmd, len);
      proc_processes[proc_count].cmd[len] = '\0';
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
      ProcUpdateProcesses ();
    }
}

void
ProcDraw (WINDOW *win)
{
  const unsigned disp_count = (proc_count < (unsigned)(getmaxy (win) - 3))
    ? proc_count
    : (unsigned)(getmaxy (win) - 3);
  const unsigned cpu_mem_off = getmaxx (win) - 12;
  const int max_off = proc_count - getmaxy (win) + 3;
  int off = proc_cursor - getmaxy (win) + 5;
  off = sm_clamp (off, 0, max_off);

  char info[32];
  DrawWindow (win, "Processes");
  snprintf (info, 32, "%d - %d of %zu", off, off + disp_count, proc_count);
  DrawWindowInfo (win, info);

  wattron (win, A_BOLD | COLOR_PAIR (C_PROC_HEADER));
  wmove (win, 1, 1);
  waddstr (win, " PID      Command");
  wmove (win, 1, cpu_mem_off);
  waddstr (win, "CPU%  Mem%");
  wattroff (win, A_BOLD | COLOR_PAIR (C_PROC_HEADER));

  struct Process *P = proc_processes + off;
  for (size_t i = 0; i < disp_count; ++i, ++P)
    {
      if (unlikely (off + i == proc_cursor))
        {
          wattron (win, COLOR_PAIR (C_PROC_CURSOR));
          wmove (win, 2 + i, 1);
          PrintN (win, ' ', getmaxx (win) - 2);
        }
      else
        wattron (win, COLOR_PAIR (C_PROC_PROCESSES));
      wmove (win, 2 + i, 1);
      PrintN (win, ' ', cpu_mem_off);
      wmove (win, 2 + i, 1);
      wprintw (win, " %-7d  %s", P->pid, P->cmd);
      wmove (win, 2 + i, cpu_mem_off);
      wprintw (win, "%4.1f  %4.1f", P->cpu, P->mem);
      if (unlikely (off + i == proc_cursor))
        wattroff (win, COLOR_PAIR (C_PROC_CURSOR));
      else
        wattroff (win, COLOR_PAIR (C_PROC_PROCESSES));
    }
}

void
ProcResize (WINDOW *win) {
  wclear (win);
  DrawWindow (win, "Processes");
}

void
ProcCursorUp (int by)
{
  if (((int)proc_cursor - by) >= 0)
    proc_cursor -= by;
  proc_cursor_pid = proc_processes[proc_cursor].pid;
}

void
ProcCursorDown (int by)
{
  if ((proc_cursor + by) < proc_count)
    proc_cursor += by;
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
}


#include "proc.h"
#include "util.h"

#define PROC_MAX_COUNT 128

Widget proc_widget = WIDGET(Proc);

const char *proc_sort = "-pcpu";

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

void ProcInit (WINDOW *win, unsigned graph_scale) {
  (void)graph_scale;
  proc_time_passed = 3141;
  DrawWindow (win, "Processes");
  DrawWindowInfo (win, "? - ? of ?");
}

void ProcQuit ()
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
      ++proc_count;
      if (proc_count == PROC_MAX_COUNT)
        break;
    }

  pclose (ps);
}

void ProcUpdate () {
  proc_time_passed += interval.tv_sec * 1000 + interval.tv_nsec / 1000000;
  if (proc_time_passed >= 2000)
    {
      proc_time_passed = 0;
      ProcUpdateProcesses ();
    }
}

void ProcDraw (WINDOW *win) {
    const unsigned disp_count = (proc_count < (unsigned)(getmaxy (win) - 3))
      ? proc_count
      : (unsigned)(getmaxy (win) - 3);
  const ptrdiff_t off = 0;//proc_count - disp_count;
  const unsigned cpu_mem_off = getmaxx (win) - 15;

  wmove (win, 1, 1);
  waddstr (win, " PID    Command");
  wmove (win, 1, cpu_mem_off);
  waddstr (win, "CPU%  Mem%");

  struct Process *P = proc_processes + off;
  for (size_t i = 0; i < disp_count; ++i, ++P)
    {
      wmove (win, 2 + i, 1);
      wprintw (win, " %-5d  %s", P->pid, P->cmd);
      wmove (win, 2 + i, cpu_mem_off);
      //fmt_fprint (stderr, "Proc::Draw: [{d}] cpu={f:.1}% mem={f:.1}%\n", P->pid, P->cpu, P->mem);
      wprintw (win, "%4.1f  %4.1f", P->cpu, P->mem);
    }
}

void ProcResize (WINDOW *win) {
  DrawWindow (win, "Processes");
  DrawWindowInfo (win, "? - ? of ?");
}


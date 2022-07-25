#include "proc.h"
#include "util.h"
#include <curses.h>
#include <stdbool.h>

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

static bool proc_search_active;
static char proc_search_string[PROC_COMMAND_STORAGE_SIZE + 1];
static int proc_search_size;
static bool proc_search_all_selected;
static bool proc_search_show;
static bool proc_search_matches[PROC_MAX_COUNT];
static bool proc_search_single_match;
static int proc_first_match;

static void ProcUpdateProcesses ();
static void ProcSearchUpdateMatches ();

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
  proc_cursor = 0;

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

  proc_search_show = true;
  ProcSearchUpdateMatches ();
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
  const unsigned rows = (unsigned)getmaxy (win) - 3 - proc_search_active;
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
  bool highlight;
  unsigned row = 2;
  if (proc_search_show)
    // We don't have strnstr on Linux so we need to null-terminate the needle
    // for strstr.
    proc_search_string[proc_search_size] = '\0';
  for (; first <= last; ++first, ++row)
    {
      highlight = proc_search_show && proc_search_matches[first];
      if (unlikely (first == proc_cursor))
        wattron (win, COLOR_PAIR (C_PROC_CURSOR));
      else if (highlight)
        wattron (win, COLOR_PAIR (C_PROC_HIGHLIGHT) | A_DIM);
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
      else if (highlight)
        wattroff (win, COLOR_PAIR (C_PROC_HIGHLIGHT) | A_DIM);
      else
        wattroff (win, COLOR_PAIR (C_PROC_PROCESSES));
    }

  /* Search */
  if (proc_search_active)
    {
      wmove (win, row, 1);
      PrintN (win, ' ', getmaxx (win) - 2);
      wmove (win, row, 1);
      wattron (win, A_BOLD);
      waddstr (win, "Search: ");
      wattroff (win, A_BOLD);
      if (proc_search_all_selected)
        wattron (win, A_REVERSE);
      waddnstr (win, proc_search_string, proc_search_size);
      if (proc_search_all_selected)
        wattroff (win, A_REVERSE);
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
  proc_time_passed = 2000;
}

void
ProcToggleTree ()
{
  proc_ps_tree = !proc_ps_tree;
  proc_time_passed = 2000;
}

bool
ProcSearching ()
{
  return proc_search_active;
}

void
ProcBeginSearch ()
{
  proc_search_active = true;
  proc_search_size = 0;
  proc_search_all_selected = false;
  curs_set (1);
}

static void
ProcSearchUpdateMatches ()
{
  size_t i;
  unsigned count = 0;
  proc_first_match = -1;
  if (proc_search_size == 0 || !proc_search_show)
    return;
  // We don't have strnstr on Linux so we need to null-terminate the needle
  // for strstr.
  proc_search_string[proc_search_size] = '\0';
  for (i = 0; i < proc_count; ++i)
    {
      proc_search_matches[i] = strstr (proc_processes[i].cmd, proc_search_string) != NULL;
      if (proc_search_matches[i])
        {
          ++count;
          if (proc_first_match == -1)
            proc_first_match = i;
        }
    }
  proc_search_single_match = count == 1;
  proc_search_show = count > 0;
}

static void
ProcSearchCommit ()
{
  proc_search_active = false;
  ProcSearchUpdateMatches ();
  curs_set (0);
}

void
ProcSearchHandleInput (int key)
{
  int i;
  const int size_before = proc_search_size;
  switch (key)
    {
    case KEY_BACKSPACE:
      if (proc_search_all_selected)
        {
          proc_search_size = 0;
          proc_search_all_selected = false;
        }
      else if (proc_search_size)
        --proc_search_size;
      break;

    case KEY_CTRL_BACKSPACE:
      if (proc_search_all_selected)
        {
          proc_search_size = 0;
          proc_search_all_selected = false;
        }
      else if (proc_search_size)
        {
          for (i = proc_search_size - 1; i >= 0; --i)
            {
              if (!isalnum (proc_search_string[i]))
                break;
            }
          proc_search_size = i + 1;
        }
      break;

    case KEY_CTRL_A:
      proc_search_all_selected = true;
      break;

    case KEY_ENTER:
      ProcSearchCommit ();
      break;

    case KEY_ESCAPE:
      proc_search_size = 0;
      ProcSearchCommit ();
      break;

    default:
      if (isprint (key))
        {
          if (proc_search_all_selected)
            {
              proc_search_size = 0;
              proc_search_all_selected = false;
            }
          if (proc_search_size < PROC_COMMAND_STORAGE_SIZE)
            proc_search_string[proc_search_size++] = key;
        }
      else
        {
          proc_search_all_selected = false;
          return;
        }
      break;
    }

  proc_search_show = proc_search_size > 0;
  if (proc_search_size != size_before)
    {
      ProcSearchUpdateMatches ();
      if (proc_search_show)
        {
          proc_cursor = proc_first_match;
          proc_cursor_pid = proc_processes[proc_cursor].pid;
        }
    }
  proc_time_passed = 2000;
}

void
ProcSearchNext ()
{
  const unsigned stop = proc_cursor;
  if (!proc_search_show)
    return;
  else if (proc_search_single_match)
    {
      proc_cursor = proc_first_match;
      proc_cursor_pid = proc_processes[proc_cursor].pid;
      return;
    }
  ++proc_cursor;
  if (proc_cursor == proc_count)
    proc_cursor = 0;
  while (!proc_search_matches[proc_cursor] && proc_cursor != stop)
    {
      ++proc_cursor;
      if (proc_cursor == proc_count)
        proc_cursor = 0;
    }
  proc_cursor_pid = proc_processes[proc_cursor].pid;
}

void
ProcSearchPrev ()
{
  const unsigned stop = proc_cursor;
  if (!proc_search_show)
    return;
  else if (proc_search_single_match)
    {
      proc_cursor = proc_first_match;
      proc_cursor_pid = proc_processes[proc_cursor].pid;
      return;
    }
  if (proc_cursor)
    --proc_cursor;
  else
    proc_cursor = proc_count - 1;
  while (!proc_search_matches[proc_cursor] && proc_cursor != stop)
    {
      if (proc_cursor)
        --proc_cursor;
      else
        proc_cursor = proc_count - 1;
    }
  proc_cursor_pid = proc_processes[proc_cursor].pid;
}


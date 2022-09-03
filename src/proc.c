#include "proc.h"
#include "util.h"
#include "sm.h"
#include "input.h"

#define PROC_MAX_COUNT 512

#define PROC_COMMAND_STORAGE_SIZE 128

Widget proc_widget = WIDGET("processes", Proc);

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
static unsigned proc_view_begin;
static unsigned proc_view_size;

static bool proc_search_active;
static History *proc_search_history;
static Input_String *proc_search_string;
static bool proc_search_show;
static bool proc_search_matches[PROC_MAX_COUNT];
static bool proc_search_single_match;
static int proc_first_match;

static void ProcUpdateProcesses ();
static void ProcSearchUpdateMatches ();

static void
ProcSetCursor (unsigned cursor)
{
  if (cursor < proc_cursor && cursor < proc_view_begin + 1)
    proc_view_begin = cursor ? cursor - 1 : 0;
  else if (cursor > proc_cursor)
    {
      if (cursor > proc_view_begin + proc_view_size - 2)
        proc_view_begin
          = cursor - proc_view_size + 1 + (cursor != proc_count - 1);
    }
  proc_cursor = cursor;
  proc_cursor_pid = proc_processes[cursor].pid;
}

static inline void
ProcSetViewSize (unsigned size)
{
  proc_view_size = size;
  proc_page_move_amount = size / 2;
  ProcSetCursor (proc_cursor);
}

void
ProcInit (WINDOW *win, unsigned graph_scale) {
  (void)graph_scale;
  proc_time_passed = 0;
  DrawWindow (win, "Processes");
  pthread_mutex_init (&proc_data_mutex, NULL);
  proc_cursor_pid = -1;
  ProcUpdateProcesses ();
  proc_cursor_pid = proc_processes[0].pid;
  proc_search_history = NewHistory ();
  proc_search_string = NULL;
  proc_view_begin = 0;
  ProcSetViewSize (getmaxy (win) - 3);
}

void
ProcQuit ()
{
  pthread_mutex_destroy (&proc_data_mutex);
  DeleteHistory (proc_search_history);
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
  const int cursor_position_in_view = proc_cursor - proc_view_begin;

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
        //proc_cursor = proc_count;
        ProcSetCursor (proc_count);

      ++proc_count;
      if (proc_count == PROC_MAX_COUNT)
        break;
    }
  pclose (ps);

  proc_view_begin = proc_cursor - cursor_position_in_view;

  if (proc_count < proc_view_size)
    ProcSetViewSize (proc_count);

  if (proc_cursor >= proc_count)
    ProcSetCursor (proc_count - 1);

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

static inline void
ProcDrawBorderImpl (WINDOW *win, unsigned first, unsigned last)
{
  char info[32];
  DrawWindow (win, "Processes");
  snprintf (info, 32, "%u - %u of %zu", first+1, last+1, proc_count);
  DrawWindowInfo (win, info);
  DrawWindowInfo2 (win, "Press ? for help");
}

void
ProcDraw (WINDOW *win)
{
  pthread_mutex_lock (&proc_data_mutex);
  const unsigned cpu_mem_off = getmaxx (win) - 13;
  const unsigned first = proc_view_begin;
  const unsigned last = proc_view_begin + proc_view_size - 1;

  /* Window */
  ProcDrawBorderImpl (win, first, last);

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
  for (unsigned i = first; i <= last; ++i, ++row)
    {
      highlight = proc_search_show && proc_search_matches[i];
      if (unlikely (i == proc_cursor))
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
               proc_processes[i].pid,
               cpu_mem_off - 10, proc_processes[i].cmd);
      wmove (win, row, cpu_mem_off);
      wprintw (win, "%5.1f %5.1f",
               proc_processes[i].cpu, proc_processes[i].mem);
      /*====*/
      if (unlikely (i == proc_cursor))
        wattroff (win, COLOR_PAIR (C_PROC_CURSOR));
      else if (highlight)
        wattroff (win, COLOR_PAIR (C_PROC_HIGHLIGHT) | A_DIM);
      else
        wattroff (win, COLOR_PAIR (C_PROC_PROCESSES));
    }

  if (proc_search_active)
    GetLineDraw ();

  pthread_mutex_unlock (&proc_data_mutex);
}

void
ProcResize (WINDOW *win) {
  wclear (win);
  ProcSetViewSize (getmaxy (win) - 3 - proc_search_active);
}

void
ProcCursorUp ()
{
  if (proc_cursor)
    ProcSetCursor (proc_cursor - 1);
}

void
ProcCursorDown ()
{
  if ((proc_cursor + 1) < proc_count)
    ProcSetCursor (proc_cursor + 1);
}

void
ProcCursorPageUp ()
{
  if (proc_cursor >= proc_page_move_amount)
    ProcSetCursor (proc_cursor - proc_page_move_amount);
  else
    ProcSetCursor (0);
}

void
ProcCursorPageDown ()
{
  if ((proc_cursor + proc_page_move_amount) < proc_count)
    ProcSetCursor (proc_cursor + proc_page_move_amount);
  else
    ProcSetCursor (proc_count - 1);
}

void
ProcCursorTop ()
{
  ProcSetCursor (0);
}

void
ProcCursorBottom ()
{
  ProcSetCursor (proc_count - 1);
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

static inline void
ProcRefresh ()
{
  if (proc_widget.exists && !proc_widget.hidden)
    {
      pthread_mutex_lock (&draw_mutex);
      ProcDraw (proc_widget.win);
      wrefresh (proc_widget.win);
      pthread_mutex_unlock (&draw_mutex);
    }
}

static void
ProcSearchUpdateMatches ()
{
  size_t i;
  unsigned count = 0;
  proc_first_match = -1;
  if (StrEmpty (proc_search_string) || !proc_search_show)
    {
      proc_search_show = false;
      return;
    }
  for (i = 0; i < proc_count; ++i)
    {
      proc_search_matches[i] = (strstr (proc_processes[i].cmd,
                                        proc_search_string->data)
                                != NULL);
      if (proc_search_matches[i])
        {
          ++count;
          if (proc_first_match == -1)
            proc_first_match = i;
        }
    }
  proc_search_single_match = count == 1;
  if ((proc_search_show = count > 0) && proc_search_active)
    ProcSetCursor (proc_first_match);
}

static void
ProcSearchUpdate (Input_String *s, bool did_change)
{
  if (did_change)
    {
      proc_search_string = s;
      ProcSearchUpdateMatches ();
    }
  proc_time_passed = 2000;
  ProcRefresh ();
}

static void
ProcSearchFinish (Input_String *s)
{
  ProcSetViewSize (proc_view_size + 1);
  ProcSearchUpdateMatches ();
  proc_search_active = false;
  proc_search_string = s;
  proc_time_passed = 2000;
  ProcRefresh ();
}

void
ProcBeginSearch ()
{
  proc_search_active = true;
  ProcSetViewSize (proc_view_size - 1);
  WINDOW *win = proc_widget.win;
  const int x = 9;  // '|Search: '
  const int y = getmaxy (win) - 2;
  GetLineBegin (win, x, y, true, getmaxx (win) - x - 2,
                proc_search_history, ProcSearchUpdate, ProcSearchFinish);
  wmove (win, y, 1);
  wattron (win, A_BOLD);
  waddstr (win, "Search: ");
  wattroff (win, A_BOLD);
}

void
ProcSearchNext ()
{
  const unsigned stop = proc_cursor;
  unsigned cursor = proc_cursor;
  if (!proc_search_show)
    return;
  else if (proc_search_single_match)
    {
      ProcSetCursor (proc_first_match);
      return;
    }
  ++cursor;
  if (cursor == proc_count)
    cursor = 0;
  while (!proc_search_matches[cursor] && cursor != stop)
    {
      ++cursor;
      if (cursor == proc_count)
        cursor = 0;
    }
  ProcSetCursor (cursor);
}

void
ProcSearchPrev ()
{
  const unsigned stop = proc_cursor;
  unsigned cursor = proc_cursor;
  if (!proc_search_show)
    return;
  else if (proc_search_single_match)
    {
      ProcSetCursor (proc_first_match);
      return;
    }
  if (cursor)
    --cursor;
  else
    cursor = proc_count - 1;
  while (!proc_search_matches[cursor] && cursor != stop)
    {
      if (cursor)
        --cursor;
      else
        cursor = proc_count - 1;
    }
  ProcSetCursor (cursor);
}

void
ProcMinSize (int *width_return, int *height_return)
{
  *width_return = 7+7+5+5+8;
  //              \ \ \ \ \_ Total spacing and padding
  //               \ \ \ \__ Memory
  //                \ \ \___ CPU
  //                 \ \____ Command
  //                  \_____ Pid
  // Header, 2 rows of processes (1 for search)
  *height_return = 3;
}

bool
ProcHandleInput (int key)
{
  switch (key)
    {
    case KEY_UP:
    case 'k':
      ProcCursorUp ();
      break;
    case KEY_DOWN:
    case 'j':
      ProcCursorDown ();
      break;
    case 'K':
    case KEY_PPAGE:
      ProcCursorPageUp ();
      break;
    case 'J':
    case KEY_NPAGE:
      ProcCursorPageDown ();
      break;
    case 'g':
    case KEY_HOME:
      ProcCursorTop ();
      break;
    case 'G':
    case KEY_END:
      ProcCursorBottom ();
      break;
    case 'p':
      ProcSetSort (PROC_SORT_PID);
      break;
    case 'P':
      ProcSetSort (PROC_SORT_INVPID);
      break;
    case 'c':
      ProcSetSort (PROC_SORT_CPU);
      break;
    case 'm':
      ProcSetSort (PROC_SORT_MEM);
      break;
    case 'f':
      ProcToggleTree ();
      break;
    case '/':
      ProcBeginSearch ();
      break;
    case 'n':
      ProcSearchNext ();
      break;
    case 'N':
      ProcSearchPrev ();
      break;
    default:
      return false;
    }
  ProcRefresh ();
  return true;
}

void
ProcDrawBorder (WINDOW *win)
{
  const unsigned first = proc_view_begin;
  const unsigned last = proc_view_begin + proc_view_size - 1;
  ProcDrawBorderImpl (win, first, last);
}

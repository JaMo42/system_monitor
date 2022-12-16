#include "proc.h"
#include "util.h"
#include "sm.h"
#include "input.h"
#include "ps/ps.h"
#include "context_menu.h"
#include "dialog.h"

bool proc_forest = false;
bool proc_kthreads = false;

Widget proc_widget = WIDGET("processes", Proc);

extern struct timespec interval;
static unsigned long proc_time_passed;
static pthread_mutex_t proc_data_mutex;
static int current_sorting_mode = PS_SORT_CPU_DESCENDING;
static size_t proc_count;

static unsigned proc_cursor;
static pid_t proc_cursor_pid;
static unsigned proc_view_begin;
static unsigned proc_view_size;
static unsigned proc_page_move_amount;

static bool proc_search_active;
static bool proc_search_active;
static History *proc_search_history;
static Input_String *proc_search_string;
static bool proc_search_show;
static bool proc_search_single_match;
static int proc_first_match;
static int proc_last_match;
static pid_t proc_cursor_pid_before_search;

static Context_Menu proc_context_menu;

static void
DrawHeader (WINDOW *win)
{
  const unsigned cpu_mem_off = getmaxx (win) - 13;
  static const char *const pid_indicators[] = {"▼", "▲", " ", " ", " ", " "};
  static const char *const cpu_indicators[] = {" ", " ", "▼", "▲", " ", " "};
  static const char *const mem_indicators[] = {" ", " ", " ", " ", "▼", "▲"};

  wattron (win, A_BOLD | COLOR_PAIR (C_PROC_HEADER));

  wmove (win, 1, 1);
  waddstr (win, " PID");
  waddstr (win, pid_indicators[current_sorting_mode]);

  wmove (win, 1, 11);
  waddstr (win, "Command");

  wmove (win, 1, cpu_mem_off);
  waddstr (win, cpu_indicators[current_sorting_mode]);
  waddstr (win, "CPU%");

  wmove (win, 1, cpu_mem_off + 6);
  waddstr (win, mem_indicators[current_sorting_mode]);
  waddstr (win, "MEM%");

  wattroff (win, A_BOLD | COLOR_PAIR (C_PROC_HEADER));
}

static inline void
ProcSetCursor (unsigned cursor)
{
  // Lines visible above/below the cursor
  const unsigned lines_before = 5;
  // Top
  if (cursor < lines_before)
    proc_view_begin = 0;
  // Bottom
  else if (cursor >= proc_count - lines_before)
    proc_view_begin = proc_count - proc_view_size;
  // Scrolling up
  else if (cursor < proc_cursor && cursor < proc_view_begin + lines_before)
    proc_view_begin = cursor - lines_before;
  // Scrolling down
  else if (cursor > proc_cursor
           && cursor >= proc_view_begin + proc_view_size - lines_before)
    proc_view_begin = cursor - proc_view_size + lines_before + 1;
  if (proc_view_begin + proc_view_size > proc_count)
    proc_view_begin = proc_count - proc_view_size;
  proc_cursor = cursor;
  proc_cursor_pid = ps_get_procs ()[cursor]->pid;
}

static inline void
ProcSetViewSize (unsigned size)
{
  proc_view_size = size;
  proc_page_move_amount = size / 2;
  ProcSetCursor (proc_cursor);
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
  VECTOR(Proc_Data*) procs = ps_get_procs ();
  for (i = 0; i < vector__size (procs); ++i)
    {
      procs[i]->search_match = (strstr (procs[i]->command_line,
                                        proc_search_string->data)
                                != NULL);
      if (procs[i]->search_match)
        {
          ++count;
          if (proc_first_match == -1)
            proc_first_match = i;
          proc_last_match = i;
        }
    }
  proc_search_single_match = count == 1;
  if ((proc_search_show = count > 0) && proc_search_active)
    ProcSetCursor (proc_first_match);
}

static void
ProcFitView ()
{
  // Expand view if there is more space in the window
  const unsigned view_space = (getmaxy (proc_widget.win) - 3
                               - proc_search_active);
  if (proc_view_size < view_space)
    ProcSetViewSize (Min (view_space, proc_count));
  // Move view forward if it goes over the end of the process list
  const unsigned view_end = proc_view_begin + proc_view_size;
  if (view_end > proc_count)
    proc_view_begin = proc_count - proc_view_size;
}

static void
ProcUpdateProcesses ()
{
  VECTOR(Proc_Data*) procs = ps_get_procs ();
  proc_count = vector_size (procs);
  const unsigned cursor_position_in_view = proc_cursor - proc_view_begin;
  for (size_t i = 0; i < proc_count; ++i)
    {
      if (procs[i]->pid == proc_cursor_pid)
        {
          proc_view_begin = i - cursor_position_in_view;
          ProcSetCursor (i);
          break;
        }
    }

  if (proc_count < proc_view_size)
    ProcSetViewSize (proc_count);
  else
    ProcFitView ();

  if (proc_cursor >= proc_count)
    ProcSetCursor (proc_count - 1);

  proc_search_show = true;
  ProcSearchUpdateMatches ();
}

void
ProcInit (WINDOW *win, unsigned graph_scale)
{
  (void)graph_scale;
  proc_time_passed = 1001;
  DrawWindow (win, "Processes");
  DrawHeader (win);
  pthread_mutex_init (&proc_data_mutex, NULL);
  ps_init ();
  if (proc_forest)
    ps_toggle_forest ();
  if (proc_kthreads)
    ps_toggle_kthreads ();
  proc_view_begin = proc_cursor = 0;
  ProcSetViewSize (getmaxy (win) - 3);
  ProcUpdateProcesses ();
  static const char *menu_items[] = {
    "Stop", "Continue", "End", "Kill"
  };
  proc_context_menu = ContextMenuCreate (menu_items, countof (menu_items));
}

void
ProcQuit ()
{
  ps_quit ();
  pthread_mutex_destroy (&proc_data_mutex);
}

void
ProcUpdate () {
  proc_time_passed += interval.tv_sec * 1000 + interval.tv_nsec / 1000000;
  if (proc_time_passed < 2000)
    return;
  proc_time_passed = 0;
  pthread_mutex_lock (&proc_data_mutex);
  ps_update ();
  ProcUpdateProcesses ();
  pthread_mutex_unlock (&proc_data_mutex);
}

static int
ProcPrintPrefix (WINDOW *win, int8_t *prefix, unsigned level, bool color)
{
  static const char *prefixes[] = {
    "│   ",
    "    ",
    "├─ ",
    "├─ ... ",
    "╰─ ",
    "╰─ ... ",
    "╎ ",
  };
  // Display width of the prefixes
  static const int prefix_sizes[] = {4, 4, 3, 7, 3, 7, 2};
  int width = 0;
  if (color)
    PushStyle (win, 0, C_PROC_BRANCHES);
  if (level < PS_MAX_LEVEL)
    {
      for (unsigned i = 0; i < level; ++i)
        {
          width += prefix_sizes[prefix[i]];
          waddstr (win, prefixes[prefix[i]]);
        }
    }
  else
    {
      for (unsigned i = 0; i < PS_MAX_LEVEL-1; ++i)
        {
          width += prefix_sizes[prefix[i]];
          waddstr (win, prefixes[prefix[i]]);
        }
      width += prefix_sizes[PS_PREFIX_MAX_LEVEL];
      waddstr (win, prefixes[PS_PREFIX_MAX_LEVEL]);
    }
  if (color)
    PopStyle (win);
  return width;
}

static void
ProcFormatPercentage (WINDOW *win, unsigned long value, unsigned long max_value)
{
  if (max_value == 0)
    {
      waddstr (win, "   ?");
      return;
    }
  unsigned long p = value * 1000UL / max_value;
  if (p > 999)
    {
      wattron (win, COLOR_PAIR (C_PROC_HIGH_PERCENT));
      waddstr (win, " 100");
      wattroff (win, COLOR_PAIR (C_PROC_HIGH_PERCENT));
    }
  else
    {
      if (p > 900) {
        wattron (win, COLOR_PAIR (C_PROC_HIGH_PERCENT));
      }
      wprintw (win, "%2u.%u", (unsigned)(p / 10), (unsigned)(p % 10));
      if (p > 900) {
        wattroff (win, COLOR_PAIR (C_PROC_HIGH_PERCENT));
      }
    }
}

static void
ProcDrawBorderImpl (WINDOW *win)
{
  DrawWindow (win, "Processes");
  if (proc_count)
    {
      char info[32];
      snprintf (info, 32, "%u - %u of %zu", proc_view_begin,
                proc_view_begin + proc_view_size, proc_count);
      DrawWindowInfo (win, info);
    }
  DrawWindowInfo2 (win, "Press ? for help");
}

void
ProcDraw (WINDOW *win)
{
  const int cpu_and_memory_offset = 12;
  pthread_mutex_lock (&proc_data_mutex);

  ProcDrawBorderImpl (win);

  const unsigned long cpu_period = ps_cpu_period ();
  const unsigned long total_memory = ps_total_memory ();
  int command_len = (getmaxx (win) - 1 - 1 - 2 - 7 - 2 - cpu_and_memory_offset);
  int prefix_size;
  unsigned row = 2;
  bool colorize_branches;
  VECTOR(Proc_Data*) procs = ps_get_procs ();
  const unsigned end = proc_view_begin + proc_view_size;
  for (unsigned i = proc_view_begin; i < end; ++i, ++row)
    {
      Proc_Data *const p = procs[i];
      colorize_branches = false;
      if (i == proc_cursor)
        wattrset (win, COLOR_PAIR (C_PROC_CURSOR));
      else if (proc_search_show && p->search_match)
        wattrset (win, COLOR_PAIR (C_PROC_HIGHLIGHT));
      else
        {
          wattrset (win, COLOR_PAIR (C_PROC_PROCESSES));
          colorize_branches = true;
        }
      wmove (win, row, 1);
      PrintN (win, ' ', getmaxx (win) - 2);
      wmove (win, row, 2);
      wprintw (win, "%7d  ", p->pid);
      prefix_size = ProcPrintPrefix (win, p->tree_prefix, p->tree_level,
                                     colorize_branches);
      waddnstr (win, p->command_line, command_len - prefix_size);
      wmove (win, row, getmaxx (win) - cpu_and_memory_offset);
      ProcFormatPercentage (win, p->total_cpu_time, cpu_period);
      waddstr (win, "  ");
      ProcFormatPercentage (win, p->total_memory, total_memory);
    }
  wattrset (win, A_NORMAL);
  const unsigned row_end = getmaxy (win) - 1 - proc_search_active;
  for (; row < row_end; ++row)
    {
      wmove (win, row, 1);
      PrintN (win, ' ', getmaxx (win) - 2);
    }

  if (proc_search_active)
    GetLineDraw ();

  pthread_mutex_unlock (&proc_data_mutex);
}

void
ProcResize (WINDOW *win)
{
  wclear (win);
  DrawHeader (win);
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

static inline void
ProcRefresh (bool redraw_header)
{
  if (proc_widget.exists && !proc_widget.hidden)
    {
      pthread_mutex_lock (&draw_mutex);
      ProcDraw (proc_widget.win);
      if (redraw_header)
        DrawHeader (proc_widget.win);
      wrefresh (proc_widget.win);
      pthread_mutex_unlock (&draw_mutex);
    }
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
  ProcRefresh (false);
}

static void
ProcSearchFinish (Input_String *s)
{
  ProcSetViewSize (proc_view_size + 1);
  ProcSearchUpdateMatches ();
  proc_search_active = false;
  proc_search_string = s;
  proc_time_passed = 2000;
  if (StrEmpty (s))
    {
      VECTOR(Proc_Data*) procs = ps_get_procs ();
      for (size_t i = 0; i < proc_count; ++i)
        {
          if (procs[i]->pid == proc_cursor_pid_before_search)
            {
              proc_cursor = i;
              proc_cursor_pid = proc_cursor_pid_before_search;
              break;
            }
        }
    }
  ProcRefresh (false);
}

void
ProcBeginSearch ()
{
  proc_cursor_pid_before_search = proc_cursor_pid;
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

void
ProcSearchNext ()
{
  if (!proc_search_show)
    return;
  unsigned cursor;
  if (proc_search_single_match)
    cursor = proc_first_match;
  else if ((int)proc_cursor >= proc_last_match)
    cursor = proc_first_match;
  else
    {
      // proc_cursor cannot be the last process due to the above statement
      cursor = proc_cursor + 1;
      VECTOR(Proc_Data*) procs = ps_get_procs ();
      // We also know there is another match after the cursor
      while (!procs[cursor]->search_match)
        ++cursor;
    }
  ProcSetCursor (cursor);
}

void
ProcSearchPrev ()
{
  if (!proc_search_show)
    return;
  unsigned cursor;
  if (proc_search_single_match)
    cursor = proc_first_match;
  else if ((int)proc_cursor <= proc_first_match)
    cursor = proc_last_match;
  else
    {
      // proc_cursor cannot be zero due to the above statement
      cursor = proc_cursor - 1;
      VECTOR(Proc_Data*) procs = ps_get_procs ();
      // We also know there is another match before the cursor
      while (!procs[cursor]->search_match)
        --cursor;
    }
  ProcSetCursor (cursor);
}

static inline void
ProcShowContextMenu (int x)
{
  if (x == -1)
    x = getbegx (proc_widget.win) + 11;
  else
    x += getbegx (proc_widget.win);
  const int y = getbegy (proc_widget.win) + 4 + (proc_cursor - proc_view_begin);
  // Stop, Continue, End, Kill
  int signal = 0;
  switch (ContextMenuShow (&proc_context_menu, x, y))
    {
    case 0: signal = SIGSTOP; break;
    case 1: signal = SIGCONT; break;
    case 2: signal = SIGTERM; break;
    case 3: signal = SIGKILL; break;
    }
  if (signal && kill (proc_cursor_pid, signal) == -1)
    {
      static const char *fmt = "Cannot kill process with PID %d with signal %d: \n%s";
      const char *error = strerror (errno);
      int len = snprintf (NULL, 0, fmt, proc_cursor_pid, signal, error) + 1;
      char *buf = malloc (len);
      (void)snprintf (buf, len, fmt, proc_cursor_pid, signal, error);
      ShowPlainMessageBox ("Error", buf);
      free (buf);
    }
  wrefresh (proc_widget.win);
}

bool
ProcHandleInput (int key)
{
  Proc_Data *proc;
  bool sorting_changed = false;
  // If the current mode is A, set it to B otherwise set it to A.
  #define SwapOrSetSortingType(a, b) do {    \
    if (current_sorting_mode == a) {         \
      current_sorting_mode = b;              \
    } else {                                 \
      current_sorting_mode = a;              \
    }                                        \
    pthread_mutex_lock (&proc_data_mutex);   \
    ps_set_sort (current_sorting_mode);      \
    pthread_mutex_unlock (&proc_data_mutex); \
    sorting_changed = true;                  \
  } while (0)

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
      SwapOrSetSortingType (PS_SORT_PID_DESCENDING, PS_SORT_PID_ASCENDING);
      break;
    case 'c':
      SwapOrSetSortingType (PS_SORT_CPU_DESCENDING, PS_SORT_CPU_ASCENDING);
      break;
    case 'm':
      SwapOrSetSortingType (PS_SORT_MEM_DESCENDING, PS_SORT_MEM_ASCENDING);
      break;
    case 'f':
      pthread_mutex_lock (&proc_data_mutex);
      ps_toggle_forest ();
      pthread_mutex_unlock (&proc_data_mutex);
      break;
    case 'T':
      pthread_mutex_lock (&proc_data_mutex);
      ps_toggle_kthreads ();
      pthread_mutex_unlock (&proc_data_mutex);
      break;
    case 'S':
      pthread_mutex_lock (&proc_data_mutex);
      ps_toggle_sum_children ();
      pthread_mutex_unlock (&proc_data_mutex);
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
    case ' ':
    case '\n':
      ProcShowContextMenu (-1);
      break;
    case 't':
      proc = ps_get_procs()[proc_cursor];
      /* Do not allow folding of toplevel processes as it makes the
         implementation much simpler and is generally not that useful anyways. */
      if (proc->parent)
        {
          proc->tree_folded ^= true;
          ps_remake_forest ();
        }
      break;
    default:
      return false;
    }
  ProcUpdateProcesses ();
  ProcRefresh (sorting_changed);
  return true;
}

static bool
ProcClickHeader (int x)
{
  const int cpu_off = getmaxx (proc_widget.win) - 12;
  const int mem_off = cpu_off + 6;
  bool sorting_changed = false;
  if InRange (x, 2, 9)
    SwapOrSetSortingType (PS_SORT_PID_DESCENDING, PS_SORT_PID_ASCENDING);
  else if InRange (x, cpu_off, cpu_off + 4)
    SwapOrSetSortingType (PS_SORT_CPU_DESCENDING, PS_SORT_CPU_ASCENDING);
  else if InRange (x, mem_off, mem_off + 4)
    SwapOrSetSortingType (PS_SORT_MEM_DESCENDING, PS_SORT_MEM_ASCENDING);
  return sorting_changed;
}

void
ProcHandleMouse (Mouse_Event *event)
{
  bool sorting_changed = false;
  int maxy;
  unsigned cursor_before;
  switch (event->button)
    {
    case MOUSE_WHEEL_UP:
      ProcCursorUp ();
      break;

    case MOUSE_WHEEL_DOWN:
      ProcCursorDown ();
      break;

    case BUTTON1_CLICKED:
      if (event->y == 1)
        sorting_changed = ProcClickHeader (event->x);
      else
        {
          maxy = getmaxy (proc_widget.win) - 1 - proc_search_active;
          cursor_before = proc_cursor;
          if InRange (event->y, 2, maxy)
            {
              ProcSetCursor (proc_view_begin + event->y - 2);
              if (proc_cursor == cursor_before)
                ProcShowContextMenu (event->x);
            }
        }
      break;

    default:
      return;
    }
  ProcUpdateProcesses ();
  ProcRefresh (sorting_changed);
  #undef SwapOrSetSortingType
}

void
ProcDrawBorder (WINDOW *win)
{
  ProcDrawBorderImpl (win);
}

#include "stdafx.h"
#include "util.h"
#include "ui.h"
#include "cpu.h"
#include "memory.h"
#include "network.h"
#include "proc.h"
#include "nc-help/help.h"

struct timespec interval = { .tv_sec = 0, .tv_nsec = 500000000L };
static unsigned graph_scale = 8;
static Layout *ui;
static pthread_mutex_t draw_mutex;

static help_text_type help_text = {
      {"k/↑", "Move cursor up"},
      {"j/↓", "Move cursor down"},
      {"K/PgUp", "Move cursor up multiple items"},
      {"J/PgDn", "Move cursor down multiple items"},
      {"g/Home", "Jump to top"},
      {"G/End", "Jump to bottom"},
      {"c", "Sort by CPU usage"},
      {"m", "Sort by memort usage"},
      {"p", "Sort by PID (ascending)"},
      {"P", "Sort by PID (descending)"},
      {"f", "Toggle ASCII art process tree"},
      {"'/'", "Search processes"},
      {"n", "Select next search result"},
      {"N", "Select previous search result"},
};
static help_type help;

void CursesInit ();
void CursesUpdate ();
void CursesQuit ();
void CursesResize ();

void InitWidgets ();
void UpdateWidgets ();
void DrawWidgets ();
void TooSmall ();

bool HandleInput (int key);

void ParseArgs (int, char *const *);

void HelpShow ();

static int
my_getch ()
{
  int ch;
  ch = getch ();
  if (ch == 27)
    {
      nodelay (stdscr, true);
      if (getch () == ERR)
        {
          nodelay (stdscr, false);
          return 27;
        }
      nodelay (stdscr, false);
      ch = getch ();
      switch (ch)
        {
          case 'A': return KEY_UP;
          case 'B': return KEY_DOWN;
          case 'C': return KEY_RIGHT;
          case 'D': return KEY_LEFT;
          case 'F': return KEY_END;
          case 'H': return KEY_HOME;
          case '5': (void)getch (); return KEY_PPAGE;
          case '6': (void)getch (); return KEY_NPAGE;
        }
      return 0;
    }
  return ch;
}

static void *
UpdateThread (void *arg)
{
  bool *running = arg;
  while (*running)
    {
      UpdateWidgets ();
      pthread_mutex_lock (&draw_mutex);
      DrawWidgets ();
      CursesUpdate ();
      pthread_mutex_unlock (&draw_mutex);
      nanosleep (&interval, NULL);
    }
  return NULL;
}

int
main (int argc, char *const *argv)
{
  /*r1*/ui = UICreateLayout (2, UI_ROWS);
  Layout *c1 = UICreateLayout (2, UI_COLS);
  Layout *r2 = UICreateLayout (2, UI_ROWS);
  UIAddWidget (ui, &cpu_widget, 0, 0.333f);
  UIAddLayout (ui, c1, 1, 0.666f);
  UIAddLayout (c1, r2, 0, 0.5f);
  UIAddWidget (r2, &mem_widget, 0, 0.5f);
  UIAddWidget (r2, &net_widget, 1, 0.5f);
  UIAddWidget (c1, &proc_widget, 1, 0.5f);
  UIGetMinSize (ui);

  ParseArgs (argc, argv);
  CursesInit ();
  UIConstruct (ui);
  InitWidgets ();
  CursesUpdate ();
  help_init (&help, help_text);

  bool running = true;
  int ch;
  pthread_t update_thread;
  pthread_mutex_init (&draw_mutex, NULL);
  pthread_create (&update_thread, NULL, UpdateThread, &running);
  while (running)
    {
      ch = my_getch ();
      if (ch == KEY_RESIZE)
        {
          pthread_mutex_lock (&draw_mutex);
          UIResize (ui, COLS, LINES);
          if (ui_too_small)
            {
              TooSmall ();
              UpdateWidgets ();
            }
          CursesResize ();
          pthread_mutex_unlock (&draw_mutex);
        }
      else if (!ui_too_small)
        {
          if (ProcSearching ())
            ProcSearchHandleInput (ch);
          else
            running = HandleInput (ch);
        }
      pthread_mutex_lock (&draw_mutex);
      ProcDraw (proc_widget.win);
      wrefresh (proc_widget.win);
      pthread_mutex_unlock (&draw_mutex);
    }
  pthread_join (update_thread, NULL);
  pthread_mutex_destroy (&draw_mutex);
  help_free (&help);
  UIDeleteLayout (ui);
  CursesQuit ();
}

bool
HandleInput (int key)
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
    case 'q':
      return false;
    case '?':
      pthread_mutex_lock (&draw_mutex);
      HelpShow ();
      DrawWindow (cpu_widget.win, "CPU");
      DrawWindow (mem_widget.win, "Memory");
      DrawWindow (net_widget.win, "Network");
      CursesUpdate ();
      pthread_mutex_unlock (&draw_mutex);
      break;
    }
  return true;
}

void
CursesInit ()
{
  setlocale (LC_ALL, "");
  initscr ();
  curs_set (0);
  noecho ();
  cbreak ();
  nodelay (stdscr, FALSE);
  start_color ();
  use_default_colors ();

  for (int i = 0; i < COLORS; ++i)
    init_pair (i + 1, i, -1);
  init_pair (C_PROC_CURSOR, 0, 76);
  init_pair (C_PROC_HIGHLIGHT, 0, 81);
}

void
CursesUpdate ()
{
  refresh ();
  wrefresh (cpu_widget.win);
  wrefresh (mem_widget.win);
  wrefresh (net_widget.win);
  wrefresh (proc_widget.win);
}

void
CursesQuit ()
{
  endwin ();
  //_nc_free_and_exit ();
}

void
CursesResize ()
{
  clear ();
  refresh ();
}

void
InitWidgets ()
{
  cpu_widget.Init (cpu_widget.win, graph_scale);
  mem_widget.Init (mem_widget.win, graph_scale);
  net_widget.Init (net_widget.win, graph_scale);
  proc_widget.Init (proc_widget.win, graph_scale);
}

void
UpdateWidgets ()
{
  cpu_widget.Update ();
  mem_widget.Update ();
  net_widget.Update ();
  proc_widget.Update ();
}

void
DrawWidgets ()
{
  cpu_widget.Draw (cpu_widget.win);
  mem_widget.Draw (mem_widget.win);
  net_widget.Draw (net_widget.win);
  proc_widget.Draw (proc_widget.win);
}

void
TooSmall ()
{
  static char too_small[] = "Window too small";
  int ch;
  bool running = true;
  while (running)
    {
      clear ();
      move ((LINES - 1) / 2,
            (COLS >= (int)sizeof (too_small)
             ? ((COLS - (int)sizeof (too_small)) / 2)
             : 0));
      addstr (COLS >= (int)sizeof (too_small) ? too_small : "!");
      refresh ();
      switch (ch = my_getch ())
        {
        case 'q':
          ungetch ('q');
          running = false;
          break;

        case KEY_RESIZE:
          UIResize (ui, COLS, LINES);
          if (!ui_too_small)
            running = false;
          break;
        }
    }
}

void
Usage (FILE *stream)
{
  fputs ("Usage: sm [-a] [-r millis] [-s scale]\n", stream);
  fputs ("Options:\n", stream);
  fputs ("  -a         Show average CPU usage\n", stream);
  fputs ("  -r millis  Update interval in milliseconds\n", stream);
  fputs ("  -s scale   Graph scale\n", stream);
  fputs ("  -c         Always show CPU graph in range 0~100%\n", stream);
  fputs ("  -f         ASCII art process tree\n", stream);
  fputs ("  -h         Show help message\n", stream);
}

void
ParseArgs (int argc, char *const *argv)
{
  char opt;
  unsigned long n;
  while ((opt = getopt (argc, argv, "ar:hs:cf")) != -1)
    {
      switch (opt)
        {
          case 'a':
            cpu_show_avg = true;
            break;
          case 'r':
            n = strtoull (optarg, NULL, 10);
            interval.tv_sec = n / 1000L;
            interval.tv_nsec = (n % 1000L) * 1000000L;
            break;
          case 's':
            graph_scale = (unsigned)strtoull (optarg, NULL, 10);
            break;
          case 'c':
            cpu_scale_height = false;
            break;
          case 'f':
            ProcToggleTree ();
            break;
          case 'h':
          case '?':
          default:
            Usage (stderr);
            exit (1);
        }
    }
}

void
HelpShow ()
{
  int ch;
  bool running = true;
  help_resize_offset (&help, stdscr, 2, 2);
  help_draw (&help);
  DrawWindow (help.window, "Help");
  refresh ();
  help_refresh (&help);

  if (!help_can_scroll (&help))
    {
      my_getch ();
      return;
    }

  while (running)
    {
      switch (ch = my_getch ())
        {
        case 'j':
        case KEY_UP:
          help_move_cursor (&help, -1);
          break;
        case 'k':
        case KEY_DOWN:
          help_move_cursor (&help, 1);
          break;
        case 'g':
        case KEY_HOME:
          help_set_cursor (&help, 0);
          break;
        case 'G':
        case KEY_END:
          help_set_cursor (&help, (unsigned)-1);
          break;
        case KEY_RESIZE:
        case ' ':
        case '\n':
        case 'q':
          running = false;
          break;
        }
      if (running)
        {
          help_draw (&help);
          DrawWindow (help.window, "Help");
          refresh ();
          help_refresh (&help);
        }
    }
}


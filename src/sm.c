#include "stdafx.h"
#include "util.h"
#include "cpu.h"
#include "memory.h"
#include "network.h"

static int term_width, term_height;
static WINDOW *cpu_win, *mem_win, *net_win, *proc_win;
struct timespec interval = { .tv_sec = 0, .tv_nsec = 500000000L };

void CursesInit ();
void CursesUpdate ();
void CursesQuit ();
void CursesResize ();
void UpdateAll ();
void DrawAll ();
void ParseArgs (int, char *const *);

static void
SigWinchHandler ()
{
  struct winsize w;
  ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);
  term_width = w.ws_col;
  term_height = w.ws_row;
  CursesResize ();
}

int
main (int argc, char *const *argv)
{
  ParseArgs (argc, argv);
  CursesInit ();
  CpuInit ((term_width - 2) / cpu_graph_scale + 1);
  MemoryInit ((term_width / 2 - 2) / mem_graph_scale + 2);
  MemoryUpdate ();
  NetworkInit ((term_width / 2 - 2) / net_graph_scale + 1);

  CursesUpdate ();

  char ch = 0;
  while ((ch = getch ()) != 'q')
    {
      UpdateAll ();
      DrawAll ();
      CursesUpdate ();
#ifndef MANUAL
      nanosleep (&interval, NULL);
#endif
    }

  MemoryQuit ();
  CpuQuit ();
  CursesQuit ();
}

void
CursesInit ()
{
  setlocale(LC_ALL, "");
  initscr ();
  curs_set (0);
  noecho ();
#ifndef MANUAL
  cbreak ();
  nodelay (stdscr, TRUE);
#endif
  start_color ();
  use_default_colors ();

  for (int i = 0; i < COLORS; ++i)
    init_pair (i + 1, i, -1);

 /* Actual window positions and dimensions will be set through the initial
    `SigWinchHandler' call below. */
  cpu_win = newwin (1, 1, 0, 0);
  mem_win = newwin (1, 1, 0, 0);
  net_win = newwin (1, 1, 0, 0);
  proc_win = newwin (1, 1, 0, 0);

  signal (SIGWINCH, SigWinchHandler);
  SigWinchHandler ();

  cpu_canvas = CanvasCreate (
    getmaxx (cpu_win) - 2, getmaxy (cpu_win) - 2);
  mem_canvas = CanvasCreate (
    getmaxx (mem_win) - 2, getmaxy (mem_win) - 2);
  net_canvas = CanvasCreate (
    getmaxx (net_win) - 2, getmaxy (net_win) - 2);
}

void
CursesUpdate ()
{
  refresh ();
  wrefresh (cpu_win);
  wrefresh (mem_win);
  wrefresh (net_win);
  wrefresh (proc_win);
}

void
CursesQuit ()
{
  CanvasDelete (net_canvas);
  CanvasDelete (mem_canvas);
  CanvasDelete (cpu_canvas);
  delwin (cpu_win);
  endwin ();
  //_nc_free_and_exit ();
}

void
CursesResize ()
{
  wclear (stdscr);
  wclear (cpu_win);
  wclear (mem_win);
  wclear (net_win);
  wclear (proc_win);
#if 0
  const int term_height_2 = term_height / 2;
  const int term_height_4 = term_height_2 / 2;
  const int term_width_2 = term_width / 2;

  mvwin (cpu_win, 0, 0);
  wresize (cpu_win, term_height_2, term_width);
  DrawWindow (cpu_win, "CPU");

  mvwin (mem_win, term_height_2, 0);
  wresize (mem_win, term_height_4, term_width_2);
  DrawWindow (mem_win, "Memory");

  mvwin (net_win, term_height_2 + term_height_4, 0);
  wresize (net_win, term_height_4, term_width_2);
  DrawWindow (net_win, "Network");

  mvwin (proc_win, term_height_2, term_width_2);
  wresize (proc_win, term_height_2, term_width_2);
  DrawWindow (proc_win, "Processes");
  DrawWindowInfo (proc_win, "? - ? of ?");
#else
  const int term_height_3 = term_height / 3;
  const int term_width_2 = term_width / 2;

  mvwin (cpu_win, 0, 0);
  wresize (cpu_win, term_height_3, term_width);
  DrawWindow (cpu_win, "CPU");
  if (cpu_canvas)
    CanvasResize (cpu_canvas, getmaxx (cpu_win) - 2, getmaxy (cpu_win) - 2);

  mvwin (mem_win, term_height_3, 0);
  wresize (mem_win, term_height_3, term_width_2);
  DrawWindow (mem_win, "Memory");
  if (mem_canvas)
    CanvasResize (mem_canvas, getmaxx (mem_win) - 2, getmaxy (mem_win) - 2);

  mvwin (net_win, term_height_3 * 2, 0);
  wresize (net_win, term_height_3, term_width_2);
  DrawWindow (net_win, "Network");
  if (net_canvas)
    CanvasResize (net_canvas, getmaxx (net_win) - 2, getmaxy (net_win) - 2);
  mvwin (proc_win, term_height_3, term_width_2);
  wresize (proc_win, term_height_3 * 2, term_width_2);
  DrawWindow (proc_win, "Processes");
  DrawWindowInfo (proc_win, "? - ? of ?");
#endif
}

void
UpdateAll ()
{
  CpuUpdate ();
  MemoryUpdate ();
  NetworkUpdate ();
}

void
DrawAll ()
{
  CpuDraw (cpu_win);
  MemoryDraw (mem_win);
  NetworkDraw (net_win);
}

void
Usage (FILE *stream)
{
  fputs ("Usage: sm [-a] [-r millis] [-s scale]\n", stream);
  fputs ("Options:\n", stream);
  fputs ("  -a         Show average CPU usage\n", stream);
  fputs ("  -r millis  Update interval in milliseconds\n", stream);
  fputs ("  -s scale   Graph scale\n", stream);
  fputs ("  -h         Show help message\n", stream);
}

void
ParseArgs (int argc, char *const *argv)
{
  char opt;
  unsigned long n;
  while ((opt = getopt (argc, argv, "ar:hs:")) != -1)
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
            n = strtoull (optarg, NULL, 10);
            cpu_graph_scale = n;
            mem_graph_scale = n;
            net_graph_scale = n;
            break;
          case 'h':
          case '?':
          default:
            Usage (stderr);
            exit (1);
        }
    }
}

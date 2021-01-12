#include "stdafx.h"
#include "util.h"
#include "cpu.h"
#include "memory.h"

static int term_width, term_height;
static WINDOW *cpu_win, *mem_win, *net_win, *proc_win;

void ParseArgs (int, char *const *);
void CursesInit ();
void CursesUpdate ();
void CursesQuit ();
void CursesResize ();
void UpdateAll ();
void DrawAll ();

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

  CursesUpdate ();

  char ch = 0;
  while ((ch = getch ()) != 'q')
    {
      UpdateAll ();
      DrawAll ();
      CursesUpdate ();
    }

  MemoryQuit ();
  CpuQuit ();
  CursesQuit ();
}

void
ParseArgs (int argc, char *const *argv)
{
  char opt;
  while ((opt = getopt (argc, argv, "a")) != -1)
    {
      switch (opt)
        {
          case 'a':
            cpu_show_avg = true;
            break;
        }
    }
}

void
CursesInit ()
{
  setlocale(LC_ALL, "");
  initscr ();
  curs_set (0);
  noecho ();
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
}

void
DrawAll ()
{
  CpuDraw (cpu_win);
  MemoryDraw (mem_win);
}

#include "stdafx.h"
#include "util.h"
#include "ui.h"
#include "cpu.h"
#include "memory.h"
#include "network.h"
#include "proc.h"

static int term_width, term_height;
struct timespec interval = { .tv_sec = 0, .tv_nsec = 500000000L };
static unsigned graph_scale = 8;

void CursesInit ();
void CursesUpdate ();
void CursesQuit ();
void CursesResize ();

void InitWidgets ();
void UpdateWidgets ();
void DrawWidgets ();
void ResizeWidgets ();
void QuitWidgets ();

void ParseArgs (int, char *const *);

static void
SigWinchHandler ()
{
  struct winsize w;
  ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);
  term_width = w.ws_col;
  term_height = w.ws_row;
  CursesResize ();
  ResizeWidgets ();
}

int
main (int argc, char *const *argv)
{
  /* @TODO: only widgets for now
  //UI *ui;
  Layout *r1 = LayoutCreate (LAYOUT_ROWS);
  LayoutAddWidget (r1, &cpu_widget);
  Layout *c1 = LayoutAddLayout (r1, LAYOUT_COLS);
  Layout *r2 = LayoutAddLayout (c1, LAYOUT_ROWS);
  LayoutAddWidget (c1, &proc_widget);
  LayoutAddWidget (r2, &mem_widget);
  LayoutAddWidget (r2, &net_widget);

  //LayoutDelete (r1);

  //ui = UICreate (r1);
  */

  ParseArgs (argc, argv);
  CursesInit ();
  InitWidgets ();
  CursesUpdate ();

  char ch = 0;
  while ((ch = getch ()) != 'q')
    {
      UpdateWidgets ();
      DrawWidgets ();
      CursesUpdate ();
#ifndef MANUAL
      nanosleep (&interval, NULL);
#endif
    }

  QuitWidgets ();
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
  cpu_widget.win = newwin (1, 1, 0, 0);
  mem_widget.win = newwin (1, 1, 0, 0);
  net_widget.win = newwin (1, 1, 0, 0);
  proc_widget.win = newwin (1, 1, 0, 0);
  struct winsize w;
  ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);
  term_width = w.ws_col;
  term_height = w.ws_row;
  const int term_height_3 = term_height / 3;
  const int term_width_2 = term_width / 2;
  mvwin (cpu_widget.win, 0, 0);
  wresize (cpu_widget.win, term_height_3, term_width);
  mvwin (mem_widget.win, term_height_3, 0);
  wresize (mem_widget.win, term_height_3, term_width_2);
  mvwin (net_widget.win, term_height_3 * 2, 0);
  wresize (net_widget.win, term_height_3, term_width_2);
  mvwin (proc_widget.win, term_height_3, term_width_2);
  wresize (proc_widget.win, term_height_3 * 2, term_width_2);

  signal (SIGWINCH, SigWinchHandler);
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
  delwin (cpu_widget.win);
  delwin (mem_widget.win);
  delwin (net_widget.win);
  delwin (proc_widget.win);
  endwin ();
  //_nc_free_and_exit ();
}

void
CursesResize ()
{
  clear ();
  refresh ();
#if 0
  const int term_height_2 = term_height / 2;
  const int term_height_4 = term_height_2 / 2;
  const int term_width_2 = term_width / 2;

  mvwin (cpu_widget.win, 0, 0);
  wresize (cpu_widget.win, term_height_2, term_width);
  DrawWindow (cpu_widget.win, "CPU");

  mvwin (mem_widget.win, term_height_2, 0);
  wresize (mem_widget.win, term_height_4, term_width_2);
  DrawWindow (mem_widget.win, "Memory");

  mvwin (net_widget.win, term_height_2 + term_height_4, 0);
  wresize (net_widget.win, term_height_4, term_width_2);
  DrawWindow (net_widget.win, "Network");

  mvwin (proc_widget.win, term_height_2, term_width_2);
  wresize (proc_widget.win, term_height_2, term_width_2);
  DrawWindow (proc_widget.win, "Processes");
  DrawWindowInfo (proc_widget.win, "? - ? of ?");
#else
  const int term_height_3 = term_height / 3;
  const int term_width_2 = term_width / 2;

  mvwin (cpu_widget.win, 0, 0);
  wresize (cpu_widget.win, term_height_3, term_width);
  cpu_widget.Resize (cpu_widget.win);

  mvwin (mem_widget.win, term_height_3, 0);
  wresize (mem_widget.win, term_height_3, term_width_2);
  mem_widget.Resize (mem_widget.win);

  mvwin (net_widget.win, term_height_3 * 2, 0);
  wresize (net_widget.win, term_height_3, term_width_2);
  net_widget.Resize (net_widget.win);

  mvwin (proc_widget.win, term_height_3, term_width_2);
  wresize (proc_widget.win, term_height_3 * 2, term_width_2);
  proc_widget.Resize (proc_widget.win);
#endif
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
ResizeWidgets ()
{
  cpu_widget.Resize (cpu_widget.win);
  mem_widget.Resize (mem_widget.win);
  net_widget.Resize (net_widget.win);
  proc_widget.Resize (proc_widget.win);
}

void
QuitWidgets ()
{
  cpu_widget.Quit ();
  mem_widget.Quit ();
  net_widget.Quit ();
  proc_widget.Quit ();
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
            graph_scale = (unsigned)strtoull (optarg, NULL, 10);
            break;
          case 'h':
          case '?':
          default:
            Usage (stderr);
            exit (1);
        }
    }
}

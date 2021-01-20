#include "stdafx.h"
#include "util.h"
#include "ui.h"
#include "cpu.h"
#include "memory.h"
#include "network.h"
#include "proc.h"

struct timespec interval = { .tv_sec = 0, .tv_nsec = 500000000L };
static unsigned graph_scale = 8;
static Layout *ui;

void CursesInit ();
void CursesUpdate ();
void CursesQuit ();
void CursesResize ();

void InitWidgets ();
void UpdateWidgets ();
void DrawWidgets ();

void ParseArgs (int, char *const *);

static void
SigWinchHandler ()
{
  struct winsize w;
  ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);
  CursesResize ();
  UIResize (ui, w.ws_col, w.ws_row);
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

  ParseArgs (argc, argv);
  CursesInit ();
  UIConstruct (ui);
  InitWidgets ();
  CursesUpdate ();

  int ch = 0;
  while ((ch = getch ()) != 'q')
    {
      switch (ch)
        {
        case KEY_UP:
        case 'k':
key_up:
          ProcCursorUp ();
          break;
        case KEY_DOWN:
        case 'j':
key_down:
          ProcCursorDown ();
          break;
        case 27:
          (void)getch ();  /* Consume '[' */
          ch = getch ();
          if (ch == 'A')
            goto key_up;
          else if (ch == 'B')
            goto key_down;
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
        }
      UpdateWidgets ();
      DrawWidgets ();
      CursesUpdate ();
#ifndef MANUAL
      nanosleep (&interval, NULL);
#endif
    }

  UIDeleteLayout (ui);
  CursesQuit ();
}

void
CursesInit ()
{
  setlocale (LC_ALL, "");
  initscr ();
  curs_set (0);
  noecho ();
  cbreak ();
  nodelay (stdscr, TRUE);
  start_color ();
  use_default_colors ();

  for (int i = 0; i < COLORS; ++i)
    init_pair (i + 1, i, -1);
  init_pair (C_PROC_CURSOR, 0, 76);

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

#include "stdafx.h"
#include "sm.h"
#include "util.h"
#include "ui.h"
#include "cpu.h"
#include "memory.h"
#include "network.h"
#include "proc.h"
#include "disk.h"
#include "nc-help/help.h"
#include "layout_parser.h"
#include "input.h"

#define widgets_for_each() \
  for (Widget *const *it = widgets, *w = *it; w != NULL; w = *++it)

struct timespec interval = { .tv_sec = 0, .tv_nsec = 500000000L };
static unsigned graph_scale = 8;
static const char *layout = NULL;
Layout *ui;

static help_text_type help_text = {
  HELP_LABEL ("Processes"),
  {"k/↑", "Move cursor up"},
  {"j/↓", "Move cursor down"},
  {"K/PgUp", "Move cursor up multiple items"},
  {"J/PgDn", "Move cursor down multiple items"},
  {"g/Home", "Jump to top"},
  {"G/End", "Jump to bottom"},
  {"c", "Sort by CPU usage"},
  {"m", "Sort by memort usage"},
  {"p", "Sort by PID"},
  {"", "  Selecting the same sorting mode again toggles"},
  {"", "  between descening and ascending sorting."},
  {"f", "Toggle ASCII art process tree"},
  {"T", "Toggle visibility of kernel threads"},
  {"'/'", "Search processes"},
  {"n", "Select next search result"},
  {"N", "Select previous search result"},
  HELP_LABEL ("CPU"),
  {"C", "Toggle CPU graph range scaling"},
  {"a", "Toggle average CPU usage"}
};
static help_type help;

// All available widgets
struct Widget *all_widgets[] = {
  &cpu_widget,
  &mem_widget,
  &net_widget,
  &proc_widget,
  &disk_widget,
  NULL
};
// Widgets used in the layout
static struct Widget *widgets[countof (all_widgets)];

pthread_mutex_t draw_mutex;
bool (*HandleInput) (int key);

void CursesInit ();
void CursesUpdate ();
void CursesQuit ();
void CursesResize ();

void InitWidgets ();
void UpdateWidgets ();
void DrawWidgets ();
void TooSmall ();
void DrawBorders ();

bool MainHandleInput (int key);

void ParseArgs (int, char *const *);

void HelpShow ();

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
  ParseArgs (argc, argv);

  const bool show_current_layout = layout && strcmp (layout, "?") == 0;
  if (show_current_layout)
    layout = NULL;

  const char *layout_source;
  if (layout == NULL || *layout == '\0')
    {
      if ((layout = getenv ("SM_LAYOUT")) && *layout != '\0')
        layout_source = "SM_LAYOUT";
      else
        {
          layout = "(rows 33% c[2] (cols (rows m[1] n[0]) p[3]))";
          layout_source = "<builtin layout>";
        }
    }
  else
    layout_source = "-l";

  if (show_current_layout)
    {
      printf ("Current layout: ‘%s’\n", layout);
      return 0;
    }

  ui = ParseLayoutString (layout, layout_source);
  UIGetMinSize (ui);
  UICollectWidgets (ui, widgets);

  HandleInput = MainHandleInput;

  CursesInit ();
  UIConstruct (ui);
  InitWidgets ();
  UIUpdateSizeInfo (ui);
  CursesUpdate ();
  help_init (&help, help_text);

  bool running = true;
  int ch;
  pthread_t update_thread;
  pthread_mutex_init (&draw_mutex, NULL);
  pthread_create (&update_thread, NULL, UpdateThread, &running);
  while (running)
    {
      ch = GetChar ();
      if (ch == KEY_RESIZE)
        HandleResize ();
      running = HandleInput (ch);
    }
  pthread_join (update_thread, NULL);
  pthread_mutex_destroy (&draw_mutex);
  help_free (&help);
  UIDeleteLayout (ui);
  CursesQuit ();
}

void
HandleResize ()
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

bool
MainHandleInput (int key)
{
  switch (key)
    {
    case 'q':
      return false;

    case '?':
      pthread_mutex_lock (&draw_mutex);
      HelpShow ();
      DrawBorders ();
      CursesUpdate ();
      pthread_mutex_unlock (&draw_mutex);
      break;

    default:
      widgets_for_each ()
        {
          if (w->HandleInput (key))
            break;
        }
    }
  return true;
}

void
CursesInit ()
{
  setlocale (LC_ALL, "");
  initscr ();
  curs_set (false);
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
  widgets_for_each ()
    {
      if (!w->hidden)
        wrefresh (w->win);
    }
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
  widgets_for_each ()
    w->Init (w->win, graph_scale);
}

void
UpdateWidgets ()
{
  widgets_for_each ()
    w->Update ();
}

void
DrawWidgets ()
{
  widgets_for_each ()
    {
      if (!w->hidden)
        w->Draw (w->win);
    }
}

void
DrawBorders ()
{
  widgets_for_each ()
    {
      if (!w->hidden)
        w->DrawBorder (w->win);
    }
}

void
TooSmall ()
{
  struct wrapped_text
  {
    const char *text;
    int width;
    int height;
  } wrapped_texts[] = {
    { "Window too small", 16, 1 },
    { "Window too\0small", 10, 2 },
    { "Window\0too small", 9, 2 },
    { "Window\0too\0small", 6, 3 },
  };
  int ch, i, j, len, base_y;
  const char *line;
  bool running = true;
  while (running)
    {
      clear ();
      for (i = 0; i < (int)countof (wrapped_texts); ++i)
        {
          struct wrapped_text *t = &wrapped_texts[i];
          if (t->width <= COLS && t->height <= LINES)
            {
              base_y = (LINES - t->height) / 2;
              line = t->text;
              for (j = 0; j < t->height; ++j)
                {
                  len = strlen (line);
                  move (base_y + j, (COLS - len) / 2);
                  addnstr (line, len);
                  line += len + 1;
                }
              break;
            }
        }
      refresh ();
      switch (ch = GetChar ())
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
  fputs ("  -l layout  Specifies the layout string\n", stream);
  fputs ("  -T         Show kernel threads", stderr);
  fputs ("  -h         Show help message\n", stream);
  fputc ('\n', stream);
  fputs ("If the layout option for -l is '?' the current layout string (either\n", stream);
  fputs ("the default or the SM_LAYOUT environment variable) gets printed.\n", stream);
}

void
ParseArgs (int argc, char *const *argv)
{
  char opt;
  unsigned long n;
  while ((opt = getopt (argc, argv, "ar:hs:cfl:T")) != -1)
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
            proc_forest = true;
            break;
          case 'l':
            layout = optarg;
            break;
          case 'T':
            proc_kthreads = true;
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
      GetChar ();
      return;
    }

  while (running)
    {
      switch (ch = GetChar ())
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


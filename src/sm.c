#include "stdafx.h"
#include "sm.h"
#include "util.h"
#include "ui.h"
#include "cpu.h"
#include "memory.h"
#include "network.h"
#include "proc.h"
#include "disk.h"
#include "temp.h"
#include "battery.h"
#include "nc-help/help.h"
#include "layout_parser.h"
#include "input.h"
#include "config.h"

#define widgets_for_each() \
  for (Widget *const *it = widgets, *w = *it; w != NULL; w = *++it)

struct timespec interval = { .tv_sec = 0, .tv_nsec = 500000000L };
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
  {"t", "Collapse/expand the selected tree"},
  {"T", "Toggle visibility of kernel threads"},
  {"S", "Toggle summation of child values"},
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
  &temp_widget,
  &battery_widget,
  NULL
};
// Widgets used in the layout
static struct Widget *widgets[countof (all_widgets)];

struct Widget *bottom_right_widget = NULL;

pthread_mutex_t draw_mutex;

void *overlay_data = NULL;

void (*DrawOverlay) (void *);

bool (*HandleInput) (int key);

void LoadConfig ();

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
      if (DrawOverlay)
        DrawOverlay (overlay_data);
      pthread_mutex_unlock (&draw_mutex);
      nanosleep (&interval, NULL);
    }
  return NULL;
}

int
main (int argc, char *const *argv)
{
  ReadConfig ();
  LoadConfig ();
  ParseArgs (argc, argv);

  const bool show_current_layout = layout && strcmp (layout, "?") == 0;
  if (show_current_layout)
    layout = NULL;

  const char *layout_source;
  if (layout == NULL || *layout == '\0')
    {
      Config_Read_Value *v;
      if (HaveConfig () && (v = ConfigGet ("sm", "layout")))
        {
          layout = v->as_string ();
          layout_source = "sm.ini";
        }
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
  UIUpdateSizeInfo (ui, true);
  DrawBorders();
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
  FreeConfig ();
}

void
LoadConfig ()
{
  if (!HaveConfig ())
    return;
  Config_Read_Value *v;

  if ((v = ConfigGet ("sm", "interval")))
    {
      unsigned long n = v->as_unsigned ();
      interval.tv_sec = n / 1000L;
      interval.tv_nsec = (n % 1000L) * 1000000L;
    }

  if ((v = ConfigGet ("cpu", "show-average")))
    cpu_show_avg = v->as_bool ();
  if ((v = ConfigGet ("cpu", "scale-height")))
    cpu_scale_height = v->as_bool ();

  if ((v = ConfigGet ("proc", "forest")))
    proc_forest  = v->as_bool ();
  if ((v = ConfigGet ("proc", "show-kernel-threads")))
    proc_kthreads = v->as_bool ();
  if ((v = ConfigGet ("proc", "command-only")))
    proc_command_only = v->as_bool ();

  if ((v = ConfigGet ("disk", "vertical")))
    disk_vertical = v->as_bool ();
  if ((v = ConfigGet ("disk", "mounting-points")))
    disk_fs = v->as_string ();

  if ((v = ConfigGet("net", "auto-scale")))
    net_auto_scale = v->as_bool();

  if ((v = ConfigGet("temp", "filter")))
    temp_filter = v->as_string();
  if ((v = ConfigGet("temp", "show-average")))
    temp_show_average = v->as_bool();

  if ((v = ConfigGet("battery", "battery")))
    battery_battery = v->as_string();
  if ((v = ConfigGet("battery", "slim")))
    battery_slim = v->as_bool();
  if ((v = ConfigGet("battery", "show_status")))
    battery_show_status = v->as_string();
}

void
HandleResize ()
{
  pthread_mutex_lock (&draw_mutex);
  UIResize (ui, COLS, LINES);
  bottom_right_widget = UIBottomRightVisibleWidget(ui);
  if (ui_too_small)
    {
      TooSmall ();
      UpdateWidgets ();
    }
  CursesResize ();
  DrawHelpInfo();
  pthread_mutex_unlock (&draw_mutex);
}

bool
MainHandleInput (int key)
{
  MEVENT event;
  Mouse_Event mouse;
  switch (key)
    {
    case 'q':
      return false;

    case '?':
      HelpShow ();
      break;

    case KEY_MOUSE:
      if (getmouse (&event) == OK)
        {
          ResolveMouseEvent (&event, ui, &mouse);
          mouse.widget->HandleMouse (&mouse);
        }
      break;

    case KEY_REFRESH:
      pthread_mutex_lock (&draw_mutex);
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
  //def_shell_mode();
  initscr ();
  curs_set (FALSE);
  noecho ();
  cbreak ();
  nodelay (stdscr, FALSE);
  start_color ();
  use_default_colors ();
  keypad (stdscr, TRUE);
  mousemask (ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

  for (int i = 0; i < COLORS; ++i)
    init_pair (i + 1, i, -1);
  // These values encroach on the normal colors starting from 254
  init_pair (C_PROC_CURSOR, 0, 76);
  init_pair (C_PROC_HIGHLIGHT, 0, 81);
  init_pair (C_BATTERY_FILL, 15, 27);

  //def_prog_mode();
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
    w->Init (w->win);
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
  DrawHelpInfo();
}

void
DrawHelpInfo() {
  if (bottom_right_widget) {
    DrawWindowInfo2(bottom_right_widget->win, "Press ? for help");
    wrefresh(bottom_right_widget->win);
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
  fputs ("  -c         Always show CPU graph in range 0~100%\n", stream);
  fputs ("  -f         ASCII art process tree\n", stream);
  fputs ("  -l layout  Specifies the layout string\n", stream);
  fputs ("  -T         Show kernel threads\n", stderr);
  fputs ("  -h         Show help message\n", stream);
  fputc ('\n', stream);
  fputs ("If the layout option for -l is '?' the default layout string gets printed.\n", stream);
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

          case 'c':
            cpu_scale_height = true;
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

static void
HelpDraw (void *unused)
{
  (void)unused;
  help_draw (&help);
  DrawWindow (help.window, "Help");
  help_refresh (&help);
}

void
HelpShow ()
{
  int ch;
  bool running = true, quit_key_was_resize = false;
  help_resize_offset (&help, stdscr, 2, 2);

  pthread_mutex_lock (&draw_mutex);
  help_draw (&help);
  DrawWindow (help.window, "Help");
  refresh ();
  help_refresh (&help);
  DrawOverlay = HelpDraw;
  pthread_mutex_unlock (&draw_mutex);

  if (!help_can_scroll (&help))
    quit_key_was_resize = GetChar () == KEY_RESIZE;
  else
    {
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
              quit_key_was_resize = true;
              /* FALLTHROUGH */
            case ' ':
            case '\n':
            case 'q':
              running = false;
              goto break_out;
            }
          pthread_mutex_lock (&draw_mutex);
          help_draw (&help);
          DrawWindow (help.window, "Help");
          refresh ();
          help_refresh (&help);
          pthread_mutex_unlock (&draw_mutex);
        }
    }
break_out:;

  pthread_mutex_lock (&draw_mutex);
  DrawOverlay = NULL;
  pthread_mutex_unlock (&draw_mutex);

  flushinp ();
  if (quit_key_was_resize)
    ungetch (KEY_RESIZE);
  else
    ungetch (KEY_REFRESH);
}

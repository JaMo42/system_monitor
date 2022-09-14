#include "memory.h"
#include "util.h"
#include "canvas/canvas.h"

IgnoreInput (Memory);
Widget mem_widget = WIDGET("memory", Memory);

static List *mem_main_usage;
static List *mem_swap_usage;
static unsigned long mem_main_total;
static unsigned long mem_swap_total;
static unsigned mem_max_samples;
static unsigned mem_samples;

static Canvas *mem_canvas;
static int mem_graph_scale = 5;

static struct sysinfo mem_sysinfo;

static void
MemoryGetTotal ()
{
  sysinfo (&mem_sysinfo);
  mem_main_total = mem_sysinfo.totalram;
  mem_swap_total = mem_sysinfo.totalswap;
}

void
MemoryInit (WINDOW *win, unsigned graph_scale)
{
  mem_main_usage = list_create ();
  mem_swap_usage = list_create ();
  mem_max_samples = MAX_SAMPLES (win, graph_scale);
  mem_samples = 0;
  MemoryGetTotal ();
  mem_graph_scale = graph_scale;
  MemoryDrawBorder (win);
  mem_canvas = CanvasCreate (win);
}

void
MemoryQuit ()
{
  list_delete (mem_main_usage);
  list_delete (mem_swap_usage);
  CanvasDelete (mem_canvas);
}

void
MemoryUpdate ()
{
  const bool full = mem_samples == mem_max_samples;
  sysinfo (&mem_sysinfo);
  unsigned long main_used = (mem_sysinfo.totalram - mem_sysinfo.freeram
                             - mem_sysinfo.bufferram - mem_sysinfo.sharedram);
  unsigned long swap_used = mem_sysinfo.totalswap - mem_sysinfo.freeswap;

  List_Node *main_node, *swap_node;
  if (likely (full))
    {
      main_node = list_rotate_left (mem_main_usage);
      swap_node = list_rotate_left (mem_swap_usage);
    }
  else
    {
      main_node = list_push_back (mem_main_usage);
      swap_node = list_push_back (mem_swap_usage);
    }
  main_node->f = (double)main_used / (double)mem_main_total;
  swap_node->f = (double)swap_used / (double)mem_swap_total;

  if (unlikely (!full))
    ++mem_samples;
}

static void
MemoryDrawGraph (List *l, short color)
{
  double last_x = -1.0, last_y = -1.0;

  double x = (mem_max_samples - mem_samples) * mem_graph_scale;
  double y;

  list_for_each (l, u)
    {
      x += mem_graph_scale;
      y = (double)mem_canvas->height - (((double)mem_canvas->height - 0.25) * u->f);

      if (likely (last_y >= 0.0))
        {
          CanvasDrawLine (mem_canvas,
                          (last_x - mem_graph_scale)*2.0, last_y*4.0 - 1,
                          (x - mem_graph_scale)*2.0, y*4.0 - 1,
                          color);
        }
      last_x = x;
      last_y = y;
    }
}

void
MemoryDraw (WINDOW *win)
{
  CanvasClear (mem_canvas);

  MemoryDrawGraph (mem_swap_usage, C_MEM_SWAP);
  MemoryDrawGraph (mem_main_usage, C_MEM_MAIN);

  CanvasDraw (mem_canvas, win);

  const double main_use = mem_main_usage->back->f * mem_main_total;
  const double swap_use = mem_swap_usage->back->f * mem_swap_total;

  const int width = getmaxx (win) - 2;

#define PRINT(line, lower, title, upper)                 \
  wattron (win, COLOR_PAIR (C_MEM_##upper));             \
  wmove (win, line, 3);                                  \
  wprintw (win, #title " %3d%%",                         \
           (int)(mem_##lower##_usage->back->f * 100.0)); \
  if (width > 26)                                        \
    {                                                    \
      waddch (win, ' ');                                 \
      FormatSize (win, lower##_use, true);               \
      waddch (win, '/');                                 \
      FormatSize (win, mem_##lower##_total, false);      \
    }                                                    \
  wattroff (win, COLOR_PAIR (C_MEM_##upper))

  PRINT (2, main, Main, MAIN);
  PRINT (3, swap, Swap, SWAP);
#undef PRINT
}

void
MemoryResize (WINDOW *win)
{
  wclear (win);

  CanvasResize (mem_canvas, win);

  mem_max_samples = MAX_SAMPLES (win, mem_graph_scale);
  list_clear (mem_main_usage);
  list_clear (mem_swap_usage);
  mem_samples = 0;
}

void
MemoryMinSize (int *width_return, int *height_return)
{
  *width_return = 4+1+4+2;
  //              \ \ \ \_ Left padding
  //               \ \ \__ Name
  //                \ \___ Space
  //                 \____ Percentage
  // Main, Swap, and 1 row spacing above and below
  *height_return = 4;
}

void
MemoryDrawBorder (WINDOW *win)
{
  DrawWindow (win, "Memory");
}

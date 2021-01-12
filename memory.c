#include "memory.h"
#include "util.h"

List *mem_main_usage;
List *mem_swap_usage;
unsigned long mem_main_total;
unsigned long mem_swap_total;
size_t mem_max_samples;
size_t mem_samples;

Canvas *mem_canvas;
int mem_graph_scale = 5;

static struct sysinfo mem_sysinfo;

static void
MemoryGetTotal ()
{
  sysinfo (&mem_sysinfo);
  mem_main_total = mem_sysinfo.totalram;
  mem_swap_total = mem_sysinfo.totalswap;
}

void
MemoryInit (size_t max_samples)
{
  mem_main_usage = list_create ();
  mem_swap_usage = list_create ();
  mem_max_samples = max_samples;
  mem_samples = 0;
  MemoryGetTotal ();
}

void
MemoryQuit ()
{
  list_delete (mem_main_usage);
  list_delete (mem_swap_usage);
}

void
MemorySetMaxSamples (size_t s)
{
  mem_max_samples = s;
}

void
MemoryUpdate ()
{
  const bool shift = mem_samples == mem_max_samples;
  sysinfo (&mem_sysinfo);
  unsigned long main_used = (mem_sysinfo.totalram - mem_sysinfo.freeram
                             - mem_sysinfo.bufferram - mem_sysinfo.sharedram);
  unsigned long swap_used = mem_sysinfo.totalswap - mem_sysinfo.freeswap;

  if (shift)
    {
      list_pop_front (mem_main_usage);
      list_pop_front (mem_swap_usage);
    }
  list_push_back (mem_main_usage)->f = (double)main_used / (double)mem_main_total;
  list_push_back (mem_swap_usage)->f = (double)swap_used / (double)mem_swap_total;

  if (!shift)
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
      y = mem_canvas->height - 1 - ((mem_canvas->height - 1) * u->f);

      if (last_y >= 0.0)
        {
          CanvasDrawLine (mem_canvas,
                          (last_x - mem_graph_scale)*2.0, last_y*4.0,
                          (x - mem_graph_scale)*2.0, y*4.0,
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

  wattron (win, COLOR_PAIR (C_MEM_MAIN));
  wmove (win, 2, 3);
  wprintw (win, "Main %3d%% ", (int)(mem_main_usage->back->f * 100.0));
  FormatSize (main_use, win);
  waddch (win, '/');
  FormatSize (mem_main_total, win);
  wattroff (win, COLOR_PAIR (C_MEM_MAIN));

  wattron (win, COLOR_PAIR (C_MEM_SWAP));
  wmove (win, 3, 3);
  wprintw (win, "Swap %3d%% ", (int)(mem_swap_usage->back->f * 100.0));
  FormatSize (swap_use, win);
  waddch (win, '/');
  FormatSize (mem_swap_total, win);
  wattroff (win, COLOR_PAIR (C_MEM_SWAP));
}

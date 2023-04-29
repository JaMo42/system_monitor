#include "memory.h"
#include "util.h"
#include "canvas/canvas.h"
#include "ps/util.h"
#include "graph.h"

IgnoreInput (Memory);
IgnoreMouse (Memory);
Widget mem_widget = WIDGET("memory", Memory);


static Graph mem_graph;
static Canvas *mem_canvas;

static struct sysinfo mem_sysinfo;
static unsigned long mem_main_total;
static unsigned long mem_swap_total;

static void
MemoryGetTotal ()
{
  sysinfo (&mem_sysinfo);
  mem_main_total = mem_sysinfo.totalram;
  mem_swap_total = mem_sysinfo.totalswap;
}

void
MemoryInit (WINDOW *win)
{
  MemoryGetTotal ();
  MemoryDrawBorder (win);
  mem_canvas = CanvasCreate (win);
  unsigned graph_scale = DEFAULT_GRAPH_SCALE;
  Graph_Kind graph_kind = GRAPH_KIND_STRAIGHT;
  GetGraphOptions(mem_widget.name, &graph_kind, &graph_scale);
  GraphConstruct(&mem_graph, graph_kind, 2, graph_scale);
  GraphSetFixedRange(&mem_graph, 0.0, 1.0);
  GraphSetColors(&mem_graph, C_MEM_MAIN, C_MEM_SWAP, -1);
}

void
MemoryQuit ()
{
  GraphDestroy(&mem_graph);
  CanvasDelete (mem_canvas);
}

static unsigned long
MemoryAvailable ()
{
  static char filebuf[128];
  int fd;
  ssize_t s = -1;
  if ((fd = open ("/proc/meminfo", O_RDONLY)) < 0) {
    return 0;
  }
  s = read (fd, filebuf, sizeof (filebuf) - 1);
  close (fd);
  filebuf[s > 0 ? s : 0] = '\0';

  char *p = filebuf;
  int skiplines = 2;
  while (skiplines)
    skiplines -= *p++ == '\n';
  p += 13;  /* skip "MemAvailable:" */
  skipspace (&p);
  return str2u (&p) << 10;
}

void
MemoryUpdate ()
{
  sysinfo (&mem_sysinfo);
  unsigned long main_avail = MemoryAvailable ();
  unsigned long main_used;
  if (main_avail)
    main_used = mem_sysinfo.totalram - main_avail;
  else
    main_used = (mem_sysinfo.totalram - mem_sysinfo.freeram
                 - mem_sysinfo.bufferram - mem_sysinfo.sharedram);
  unsigned long swap_used = mem_sysinfo.totalswap - mem_sysinfo.freeswap;

  GraphAddSample(&mem_graph, 0, (double)main_used / (double)mem_main_total);
  GraphAddSample(&mem_graph, 1, (double)swap_used / (double)mem_swap_total);
}

void
MemoryDraw (WINDOW *win)
{
  CanvasClear (mem_canvas);
  GraphDraw(&mem_graph, mem_canvas, NULL, NULL);
  CanvasDraw (mem_canvas, win);

  const double main_current = GraphLastSample(&mem_graph, 0);
  const double swap_current = GraphLastSample(&mem_graph, 1);
  const double main_use = main_current * mem_main_total;
  const double swap_use = swap_current * mem_swap_total;

  const int width = getmaxx (win) - 2;

#define PRINT(line, lower, title, upper)                          \
  wattron (win, COLOR_PAIR (C_MEM_##upper));                      \
  wmove (win, line, 3);                                           \
  wprintw (win, #title " %3d%%", (int)(lower##_current * 100.0)); \
  if (width > 26)                                                 \
    {                                                             \
      waddch (win, ' ');                                          \
      FormatSize (win, lower##_use, true);                        \
      waddch (win, '/');                                          \
      FormatSize (win, mem_##lower##_total, false);               \
    }                                                             \
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
  GraphSetViewport(
    &mem_graph,
    (Rectangle) { 1, 1, getmaxx(win) - 2, getmaxy(win) - 2}
  );
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

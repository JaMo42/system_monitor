#include "cpu.h"
#include "util.h"
#include "canvas/canvas.h"

Widget cpu_widget = WIDGET(Cpu);

static List **cpu_usages;
static unsigned cpu_max_samples;
static unsigned cpu_samples;
static int cpu_count;

bool cpu_show_avg = false;
bool cpu_scale_height = true;
static Canvas *cpu_canvas;
static int cpu_graph_scale = 5;

static size_t *cpu_last_total_jiffies;
static size_t *cpu_last_work_jiffies;

void
CpuInit (WINDOW *win, unsigned graph_scale)
{
  cpu_count = get_nprocs_conf ();
  cpu_usages = (List **)malloc (sizeof (List *) * (cpu_count + 1));
  for (int i= 0; i <= cpu_count; ++i)
    cpu_usages[i] = list_create ();
  cpu_max_samples = MAX_SAMPLES (win, graph_scale);
  cpu_samples = 0;
  cpu_last_total_jiffies = calloc (cpu_count + 1, sizeof (size_t));
  cpu_last_work_jiffies = calloc (cpu_count + 1, sizeof (size_t));
  cpu_show_avg = cpu_show_avg || cpu_count > 8;
  cpu_graph_scale = graph_scale;
  DrawWindow (win, "CPU");
  cpu_canvas = CanvasCreate (win);
}

void
CpuQuit ()
{
  for (int i = 0; i <= cpu_count; ++i)
    list_delete (cpu_usages[i]);
  free (cpu_usages);
  free (cpu_last_total_jiffies);
  free (cpu_last_work_jiffies);
  CanvasDelete (cpu_canvas);
}

static double
CpuPollUsage (int id, FILE *stat)
{
  int _id = 0;
  size_t user = 0, nice = 0, system = 0;
  size_t idle = 0, iowait = 0, irq = 0, softirq = 0;
  size_t steal = 0, guest = 0, guest_nice = 0;

  if (unlikely (id == 0))
    fscanf (stat, "cpu  %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
            &user, &nice, &system, &idle, &iowait, &irq, &softirq,
            &steal, &guest, &guest_nice);
  else
    fscanf (stat, "cpu%d %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
            &_id, &user, &nice, &system, &idle, &iowait, &irq, &softirq,
            &steal, &guest, &guest_nice);

  size_t total_jiffies = (user + nice + system + idle + iowait + irq + softirq
                          + steal + guest + guest_nice);
  size_t work_jiffies = user + nice + system;

  size_t total_period = total_jiffies - cpu_last_total_jiffies[id];
  size_t work_period = work_jiffies - cpu_last_work_jiffies[id];

  cpu_last_total_jiffies[id] = total_jiffies;
  cpu_last_work_jiffies[id] = work_jiffies;

  return (double)work_period / (double)total_period;
}

void
CpuUpdate ()
{
  const bool shift = cpu_samples == cpu_max_samples;
  FILE *stat = fopen ("/proc/stat", "r");

  for (int i = 0; i <= cpu_count; ++i)
    {
      if (likely (shift))
        list_pop_front (cpu_usages[i]);
      list_push_back (cpu_usages[i])->f = CpuPollUsage (i, stat);
    }
  if (unlikely (!shift))
    ++cpu_samples;
  fclose (stat);
}

static void
CpuDrawGraph (int id, short color, double max_percent,
              void (*DrawLine) (Canvas *, double, double, double, double, short))
{
  double last_x = -1.0, last_y = -1.0;

  double x = (cpu_max_samples - cpu_samples) * cpu_graph_scale;
  double y;

  list_for_each (cpu_usages[id], u)
    {
      x += cpu_graph_scale;
      y = ((double)cpu_canvas->height
           - (((double)cpu_canvas->height - 0.25) * (u->f / max_percent)));

      if (likely (last_y >= 0.0))
        {
          DrawLine (cpu_canvas,
                    (last_x - cpu_graph_scale)*2.0, last_y*4.0 - 1,
                    (x - cpu_graph_scale)*2.0, y*4.0 - 1,
                    color);
        }
      last_x = x;
      last_y = y;
    }
}

static double
CpuMaxPercent ()
{
  if (!cpu_scale_height)
    return 1.0;
  double max = 0.0;
  if (cpu_show_avg)
    {
      list_for_each (cpu_usages[0], s)
        {
          if (s->f > max)
            max = s->f;
        }
    }
  else
    {
      for (int i = 1; i <= cpu_count; ++i)
        {
          list_for_each (cpu_usages[i], s)
            {
              if (s->f > max)
                max = s->f;
            }
        }
    }
  max = round (max * 10.0) / 10.0 + 0.1;
  if (max > 1.0)
    max = 1.0;
  return max;
}

static void
CpuDrawGraphBezir (Canvas *canvas, double x1, double y1, double x2, double y2, short color)
{
  if (x1 == x2)
    {
      CanvasDrawLine (canvas, x1, y1, x2, y2, color);
      return;
    }
  /* Arbitrary constant that defined how squiggly(?) the bezir curbe will be;
     0.0 -> straight line, 2.0 -> very shallow at beginning, vertical in the middle.
     Values less than 0.0 or greater than 2.0 will cause it to overlap itself. */
#define CONTROL_FACTOR ((2.0 + 1.618) / 3.0)
  const double dx = fabs (x1 - x2);
  const double dy = fabs (y1 - y2);
  const double percision = 0.25 / (dx > dy ? dx : dy);
  /* Control point coordinates.
     Invariant: x2 > x1 */
  const double x1_c = x1 + cpu_graph_scale * CONTROL_FACTOR;
  const double x2_c = x2 - cpu_graph_scale * CONTROL_FACTOR;
  const double y1_c = y1;
  const double y2_c = y2;
  double d, x, y;
  for (d = 0.0; d <= 1.0; d += percision)
    {
      x = (          pow (1.0 - d, 3.0)             * x1
           + 3 * d * pow (1.0 - d, 2.0)             * x1_c
           + 3 *     pow (      d, 2.0) * (1.0 - d) * x2_c
           +         pow (      d, 3.0)             * x2);
      y = (          pow (1.0 - d, 3.0)             * y1
           + 3 * d * pow (1.0 - d, 2.0)             * y1_c
           + 3 *     pow (      d, 2.0) * (1.0 - d) * y2_c
           +         pow (      d, 3.0)             * y2);
      CanvasSet (canvas, x, y, color);
    }
}

#define COLOR(i) ((i) < 8 ? C_CPU_GRAPHS[i] : ((i + 10) % 256))

void
CpuDraw (WINDOW *win)
{
  CanvasClear (cpu_canvas);

  const double max_percent = CpuMaxPercent ();
  if (cpu_show_avg)
    CpuDrawGraph (0, C_CPU_AVG, max_percent, CpuDrawGraphBezir);
  else
    {
      for (int i = cpu_count; i > 0; --i)
        CpuDrawGraph (i, COLOR (i - 1), max_percent, CpuDrawGraphBezir);
    }

  CanvasDraw (cpu_canvas, win);

  const int width = getmaxx (win);
  const int height = getmaxy (win);
  if (cpu_show_avg)
    {
      wattron (win, COLOR_PAIR (C_CPU_AVG));
      mvwprintw (win, 2, 3, "AVRG %d%%", (int)(cpu_usages[0]->back->f * 100.f));
      wattroff (win, COLOR_PAIR (C_CPU_AVG));
    }
  else
    {
      const int rows = height - 4;
      for (int i = 0; i < cpu_count; ++i)
        {
          wattron (win, COLOR_PAIR (COLOR (i)));
          mvwprintw (win, 2 + i % rows, 3 + i / rows * 10, "CPU%d %d%%", i,
                     (int)(cpu_usages[i + 1]->back->f * 100.f));
          wattroff (win, COLOR_PAIR (COLOR (i)));
        }
    }
  mvwprintw (win, 1, width - 5, "%3d%%", (int)(max_percent*100.0+0.5));
  mvwaddstr (win, height - 2, width - 3, "0%");
}

void
CpuResize (WINDOW *win)
{
  wclear (win);
  DrawWindow (win, "CPU");

  CanvasResize (cpu_canvas, win);

  cpu_max_samples = MAX_SAMPLES (win, cpu_graph_scale);
  for (int i = 0; i <= cpu_count; ++i)
    {
      // @TODO: only remove excess
      list_clear (cpu_usages[i]);
    }
  cpu_samples = 0;
}

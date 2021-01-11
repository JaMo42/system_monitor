#include "cpu.h"
#include "curses_util.h"

// size = `cpu_count + 1' to account for average usage
List **cpu_usages;
size_t cpu_max_samples;
size_t cpu_samples;
int cpu_count;

bool cpu_show_avg;
Canvas *cpu_canvas;
int cpu_graph_scale = 5;

// Total and work jiffies of the last snapshot for each processor + the average.
static size_t *cpu_last_total_jiffies;
static size_t *cpu_last_work_jiffies;

void
CpuInit (size_t max_samples)
{
  cpu_count = get_nprocs_conf ();
  cpu_usages = (List **)malloc (sizeof (List *) * (cpu_count + 1));
  for (int i= 0; i <= cpu_count; ++i)
    cpu_usages[i] = list_create ();
  cpu_max_samples = max_samples;
  cpu_samples = 0;
  cpu_last_total_jiffies = calloc (cpu_count + 1, sizeof (size_t));
  cpu_last_work_jiffies = calloc (cpu_count + 1, sizeof (size_t));
  cpu_show_avg = cpu_show_avg || cpu_count > 8;
}

void
CpuQuit ()
{
  for (int i = 0; i <= cpu_count; ++i)
    list_delete (cpu_usages[i]);
  free (cpu_usages);
  free (cpu_last_total_jiffies);
  free (cpu_last_work_jiffies);
}

void
CpuSetMaxSamples (size_t s)
{
  cpu_max_samples = s;
}

static double
CpuPollUsage (int id, FILE *stat)
{
  int _id = 0;
  size_t user = 0, nice = 0, system = 0;
  size_t idle = 0, iowait = 0, irq = 0, softirq = 0;
  size_t steal = 0, guest = 0, guest_nice = 0;

  if (id == 0)
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
      if (shift)
        list_pop_front (cpu_usages[i]);
      list_push_back (cpu_usages[i])->f = CpuPollUsage (i, stat);
    }
  if (!shift)
    ++cpu_samples;
  fclose (stat);
}

static void
CpuDrawGraph (int id, short color,
              void (*DrawLine) (Canvas *, double, double, double, double, short))
{
  double last_x = -1.0, last_y = -1.0;

  double x = (cpu_max_samples - cpu_samples) * cpu_graph_scale;
  double y;

  list_for_each (cpu_usages[id], u)
    {
      x += cpu_graph_scale;
      y = cpu_canvas->height - 1 - ((cpu_canvas->height - 1) * u->f);

      if (last_y >= 0.0)
        {
          DrawLine (cpu_canvas, (last_x - cpu_graph_scale)*2.0, last_y*4.0, (x - cpu_graph_scale)*2.0, y*4.0, color);
        }
      last_x = x;
      last_y = y;
    }
}

#define COLOR(i) ((i) < 8 ? C_GRAPH_TABLE[i] : i + 10)

void
CpuDraw (WINDOW *win)
{
  CanvasClear (cpu_canvas);
  static char label_buf[16];

  if (cpu_show_avg)
    {
      CpuDrawGraph (0, C_ACCENT, CanvasDrawLineFill);
      snprintf (label_buf, 16, "AVRG %d%%", (int)(cpu_usages[0]->back->f * 100.f));
      CanvasSetText (cpu_canvas, 1, 1, label_buf, C_ACCENT);
    }
  else
    {
      for (int i = cpu_count; i > 0; --i)
        {
          CpuDrawGraph (i - 1, COLOR (i - 1), CanvasDrawLine);
        }
      for (int i = cpu_count; i > 0; --i)
        {
          snprintf (label_buf, 16, "CPU%d %d%%", i - 1, (int)(cpu_usages[i]->back->f * 100.f));
          CanvasSetText (cpu_canvas, 1, i, label_buf, COLOR (i - 1));
        }
    }

  CanvasDraw (cpu_canvas, win);
}

#include "cpu.h"
#include "canvas/canvas.h"
#include "graph.h"
#include "ps/util.h"
#include "theme.h"
#include "util.h"

IgnoreMouse(Cpu);
Widget cpu_widget = WIDGET("cpu", Cpu);

static int cpu_count;

bool cpu_show_avg = false;
bool cpu_scale_height = false;
static Canvas *cpu_canvas;
static short *cpu_colors;

static size_t *cpu_last_total_jiffies;
static size_t *cpu_last_work_jiffies;

static Graph cpu_graph;
static Graph cpu_avg_graph;

void
CpuInit(WINDOW *win) {
    cpu_count = get_nprocs_conf();
    cpu_last_total_jiffies = calloc(cpu_count + 1, sizeof(size_t));
    cpu_last_work_jiffies = calloc(cpu_count + 1, sizeof(size_t));
    cpu_show_avg = cpu_show_avg || cpu_count > 8;
    CpuDrawBorder(win);
    cpu_canvas = CanvasCreate(win);

    cpu_colors = malloc(sizeof(short) * cpu_count);
    memcpy(cpu_colors, theme->cpu_graphs, sizeof(short) * THEME_CPU_COLORS);
    const int avail = AvailableColors();
    if (cpu_count - THEME_CPU_COLORS <= avail) {
        for (int i = THEME_CPU_COLORS; i < cpu_count; i++) {
            cpu_colors[i] = DefColor((ColorDef){16 + rand() % 216, -1});
        }
    } else if (avail) {
        short *colors = malloc(sizeof(short) * avail);
        for (int i = 0; i < avail; i++) {
            colors[i] = DefColor((ColorDef){16 + rand() % 216, -1});
        }
        for (int i = THEME_CPU_COLORS; i < cpu_count; i++) {
            cpu_colors[i] = colors[(i - THEME_CPU_COLORS) % avail];
        }
        free(colors);
    } else {
        for (int i = THEME_CPU_COLORS; i < cpu_count; i++) {
            cpu_colors[i] = theme->cpu_graphs[i % THEME_CPU_COLORS];
        }
    }

    unsigned graph_scale = DEFAULT_GRAPH_SCALE;
    Graph_Kind graph_kind = GRAPH_KIND_BEZIR;
    GetGraphOptions(cpu_widget.name, &graph_kind, &graph_scale);
    GraphConstruct(&cpu_graph, graph_kind, cpu_count, graph_scale);
    GraphSetColorsList(&cpu_graph, cpu_colors, cpu_count);
    GraphConstruct(&cpu_avg_graph, graph_kind, 1, graph_scale);
    GraphSetColors(&cpu_avg_graph, theme->cpu_avg, -1);
    if (cpu_show_avg) {
        GraphSetDynamicRange(&cpu_graph, 0.1);
        GraphSetDynamicRange(&cpu_avg_graph, 0.1);
    } else {
        GraphSetFixedRange(&cpu_graph, 0.0, 1.0);
        GraphSetFixedRange(&cpu_avg_graph, 0.0, 1.0);
    }
}

void
CpuQuit() {
    free(cpu_last_total_jiffies);
    free(cpu_last_work_jiffies);
    CanvasDelete(cpu_canvas);
    GraphDestroy(&cpu_graph);
    GraphDestroy(&cpu_avg_graph);
}

static double
CpuPollUsage(int id, FILE *stat) {
    static char buf[10 * 20 + 10 + 8];
    fgets(buf, sizeof(buf), stat);
    char *p = buf;
    skipfields(&p, 1);
    skipspace(&p);

    const size_t work_jiffies = (str2u(&p) + str2u(&p) + str2u(&p));
    const size_t total_jiffies
        = (work_jiffies + str2u(&p) + str2u(&p) + str2u(&p) + str2u(&p)
           + str2u(&p) + str2u(&p) + str2u(&p));

    const size_t total_period = total_jiffies - cpu_last_total_jiffies[id];
    const size_t work_period = work_jiffies - cpu_last_work_jiffies[id];

    cpu_last_total_jiffies[id] = total_jiffies;
    cpu_last_work_jiffies[id] = work_jiffies;

    return (double)work_period / (double)total_period;
}

void
CpuUpdate() {
    FILE *stat = fopen("/proc/stat", "r");
    for (int i = 0; i <= cpu_count; ++i) {
        const double usage = CpuPollUsage(i, stat);
        if (i == 0) {
            GraphAddSample(&cpu_avg_graph, 0, usage);
        } else {
            GraphAddSample(&cpu_graph, i - 1, usage);
        }
    }
    fclose(stat);
}

void
CpuDraw(WINDOW *win) {
    double lo, hi;
    CanvasClear(cpu_canvas);
    if (cpu_show_avg) {
        GraphDraw(&cpu_avg_graph, cpu_canvas, &lo, &hi);
    } else {
        GraphDraw(&cpu_graph, cpu_canvas, &lo, &hi);
    }
    CanvasDraw(cpu_canvas, win);

    const int width = getmaxx(win);
    const int height = getmaxy(win);
    if (cpu_show_avg) {
        const double u = GraphLastSample(&cpu_avg_graph, 0);
        wattron(win, COLOR_PAIR(theme->cpu_avg));
        mvwprintw(win, 2, 3, "AVRG %d%%", (int)(u * 100.f));
        wattroff(win, COLOR_PAIR(theme->cpu_avg));
    } else {
        const int rows = height - 4;
        for (int i = 0; i < cpu_count; ++i) {
            const int y = 2 + i % rows;
            const int x = 3 + i / rows * 10;
            const double u = GraphLastSample(&cpu_graph, i);
            wattron(win, COLOR_PAIR(cpu_colors[i]));
            mvwprintw(win, y, x, "CPU%d %d%%", i, (int)(u * 100.f));
            wattroff(win, COLOR_PAIR(cpu_colors[i]));
        }
    }
    PrintPercentage(win, width - 5, 1, hi);
    PrintPercentage(win, width - 5, height - 2, lo);
}

void
CpuResize(WINDOW *win) {
    wclear(win);
    CanvasResize(cpu_canvas, win);
    const Rectangle viewport = {1, 1, cpu_canvas->width, cpu_canvas->height};
    GraphSetViewport(&cpu_graph, viewport);
    GraphSetViewport(&cpu_avg_graph, viewport);
}

void
CpuMinSize(int *width_return, int *height_return) {
    // This is only checked at startup and since showing the average CPU usage
    // can be toggled we cannot allow the widget to be smaller if it's enabled.
#define LABELS 2 + 4 * (4 + 1 + 3 + 3)
    //              \ \  \ \ \ \_ space between labels
    //               \ \  \ \ \__ percentage (2 digits)
    //                \ \  \ \___ space
    //                 \ \  \____ CPU<n>
    //                  \ \______ 2 columns
    //                   \_______ Left padding
    // + 5 for the % range labels
    *width_return = LABELS + 5;
    // For 2 rows of CPU labels and 1 row spacing above and below
    *height_return = 4;
}

void
CpuDrawBorder(WINDOW *win) {
    DrawWindow(win, "CPU");
}

bool
CpuHandleInput(int key) {
    switch (key) {
    case 'C':
        cpu_scale_height = !cpu_scale_height;
        if (cpu_scale_height) {
            GraphSetDynamicRange(&cpu_graph, 0.1);
            GraphSetDynamicRange(&cpu_avg_graph, 0.1);
        } else {
            GraphSetFixedRange(&cpu_graph, 0.0, 1.0);
            GraphSetFixedRange(&cpu_avg_graph, 0.0, 1.0);
        }
        break;

    case 'a':
        cpu_show_avg = !cpu_show_avg;
        break;

    default:
        return false;
    }
    return true;
}

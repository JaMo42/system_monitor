#include "network.h"
#include "util.h"
#include "canvas/canvas.h"
#include "graph.h"

extern struct timespec interval;

IgnoreInput (Network);
IgnoreMouse (Network);
Widget net_widget = WIDGET("network", Network);

bool net_auto_scale = false;

static unsigned long net_receive_total;
static unsigned long net_transmit_total;

static Canvas *net_canvas;

static struct {
  char *name;
  char *rt_char;
} *net_interfaces;
static unsigned net_interface_count;
static double net_period;

static Graph net_recv_graph;
static Graph net_send_graph;

static void
NetworkGetInterfaces ()
{
  struct ifaddrs *ifaddr = NULL, *ifa = NULL;
  int count = 0, l = 0;

  getifaddrs (&ifaddr);

  for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
      if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET
          || !strcmp (ifa->ifa_name, "lo"))
        continue;
      ++count;
    }

  net_interface_count = count;
  net_interfaces = malloc (count * sizeof (*net_interfaces));

  for (ifa = ifaddr, count = 0; ifa; ifa = ifa->ifa_next)
    {
      if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET
          || !strcmp (ifa->ifa_name, "lo"))
        continue;
      l = strlen (ifa->ifa_name);
      net_interfaces[count].name = malloc (36 + l);
      memcpy (net_interfaces[count].name, "/sys/class/net/", 15);
      memcpy (net_interfaces[count].name + 15, ifa->ifa_name, l);
      memcpy (net_interfaces[count].name + 15 + l, "/statistics/?x_bytes", 20);
      net_interfaces[count].name[35 + l] = '\0';
      net_interfaces[count].rt_char = net_interfaces[count].name + 27 + l;
      ++count;
    }

  freeifaddrs (ifaddr);
}

void
NetworkInit (WINDOW *win)
{
  net_receive_total = 0;
  net_transmit_total = 0;
  NetworkGetInterfaces ();
  net_period = (double)interval.tv_sec + interval.tv_nsec / 1.0e9;
  unsigned graph_scale = DEFAULT_GRAPH_SCALE;
  Graph_Kind graph_kind = GRAPH_KIND_BLOCKS;
  GetGraphOptions(net_widget.name, &graph_kind, &graph_scale);
  GraphConstruct(&net_recv_graph, graph_kind, 1, graph_scale);
  GraphSetColors(&net_recv_graph, C_NET_RECEIVE, -1);
  GraphSetDynamicRange(&net_recv_graph, 0.1);
  GraphConstruct(&net_send_graph, graph_kind, 1, graph_scale);
  GraphSetColors(&net_send_graph, C_NET_TRANSMIT, -1);
  GraphSetDynamicRange(&net_send_graph, 0.1);
  NetworkUpdate ();
  list_clear(net_recv_graph.samples[0]);
  list_clear(net_send_graph.samples[0]);
  NetworkDrawBorder (win);
  net_canvas = CanvasCreate (win);
}

void
NetworkQuit ()
{
  GraphDestroy(&net_recv_graph);
  GraphDestroy(&net_send_graph);
  for (unsigned i = 0; i < net_interface_count; ++i)
    {
      free (net_interfaces[i].name);
    }
  free (net_interfaces);
  CanvasDelete (net_canvas);
}

static inline unsigned long
NetworkGetBytes (FILE *f)
{
  char buf[128];
  fgets (buf, 128, f);
  return strtoul (buf, NULL, 10);
}

void
NetworkUpdate ()
{
  unsigned long prev_receive = net_receive_total;
  unsigned long prev_transmit = net_transmit_total;
  FILE *f;

  net_receive_total = 0;
  net_transmit_total = 0;

  for (unsigned i = 0; i < net_interface_count; ++i)
    {
      *net_interfaces[i].rt_char = 'r';
      f = fopen (net_interfaces[i].name, "r");
      net_receive_total += NetworkGetBytes (f);
      fclose (f);

      *net_interfaces[i].rt_char = 't';
      f = fopen (net_interfaces[i].name, "r");
      net_transmit_total += NetworkGetBytes (f);
      fclose (f);
    }

  double receive_period = net_receive_total - prev_receive;
  double transmit_period = net_transmit_total - prev_transmit;

  GraphAddSample(&net_recv_graph, 0, receive_period * net_period);
  GraphAddSample(&net_send_graph, 0, transmit_period * net_period);
}

void
NetworkDraw (WINDOW *win)
{
  CanvasClear (net_canvas);
  GraphDraw(&net_recv_graph, net_canvas, NULL, NULL);
  GraphDraw(&net_send_graph, net_canvas, NULL, NULL);
  CanvasDraw (net_canvas, win);

  wmove (win, 2, 3);
  waddstr (win, "Total RX:");
  wattron (win, COLOR_PAIR (C_NET_RECEIVE));
  FormatSize (win, net_receive_total, true);
  wattroff (win, COLOR_PAIR (C_NET_RECEIVE));
  wmove (win, 3, 3);
  waddstr (win, "RX/s:    ");
  wattron (win, COLOR_PAIR (C_NET_RECEIVE));
  FormatSize (win, (size_t)GraphLastSample(&net_recv_graph, 0), true);
  waddstr (win, "/s");
  wattroff (win, COLOR_PAIR (C_NET_RECEIVE));

  wmove (win, getmaxy (win) / 2 + 1, 3);
  waddstr (win, "Total TX:");
  wattron (win, COLOR_PAIR (C_NET_TRANSMIT));
  FormatSize (win, net_transmit_total, true);
  wattroff (win, COLOR_PAIR (C_NET_TRANSMIT));
  wmove (win, getmaxy (win) / 2 + 2, 3);
  waddstr (win, "TX/s:    ");
  wattron (win, COLOR_PAIR (C_NET_TRANSMIT));
  FormatSize (win, (size_t)GraphLastSample(&net_send_graph, 0), true);
  waddstr (win, "/s");
  wattroff (win, COLOR_PAIR (C_NET_TRANSMIT));
}

void
NetworkResize (WINDOW *win)
{
  wclear (win);

  CanvasResize (net_canvas, win);

  const int width = getmaxx(win) - 2;
  const int height = getmaxy(win) - 2;
  const int mid = (height + 1) / 2;
  const int graph_height = Max(mid - 3, 1);
  const int y1 = 1 + (mid - graph_height);
  const int y2 = 1 + (height - graph_height);
  if (net_auto_scale)
    {
      const int scale = (graph_height + 3) / 4 * (2 - (graph_height == 1));
      GraphSetScale(&net_recv_graph, scale, false);
      GraphSetScale(&net_send_graph, scale, false);
    }
  GraphSetViewport(
    &net_recv_graph,
    (Rectangle) { 1, y1, width, graph_height }
  );
  GraphSetViewport(
    &net_send_graph,
    (Rectangle) { 1, y2, width, graph_height }
  );
}

void
NetworkMinSize (int *width_return, int *height_return)
{
  *width_return = 2+10+5+4;
  //              \ \ \ \_ Unit
  //               \ \ \__ Amount
  //                \ \___ Name and space
  //                 \____ Left padding
  // 2 lines for receiving and transmitting and 1 line above and below
  *height_return = 7;
}

void
NetworkDrawBorder (WINDOW *win)
{
  DrawWindow (win, "Network");
}


#include "network.h"
#include "util.h"
#include "canvas/canvas.h"

extern struct timespec interval;

IgnoreInput (Network);
Widget net_widget = WIDGET("network", Network);

List *net_receive;
List *net_transmit;
unsigned long net_receive_total;
unsigned long net_transmit_total;
unsigned net_max_samples;
unsigned net_samples;

Canvas *net_canvas;
int net_graph_scale = 5;

static struct {
  char *name;
  char *rt_char;
} *net_interfaces;
static unsigned net_interface_count;
static double net_period;
static uintmax_t net_receive_max;
static uintmax_t net_transmit_max;

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
NetworkInit (WINDOW *win, unsigned graph_scale)
{
  net_receive = list_create ();
  net_transmit = list_create ();
  net_receive_total = 0;
  net_transmit_total = 0;
  net_max_samples = MAX_SAMPLES (win, graph_scale);
  net_samples = 0;
  NetworkGetInterfaces ();
  net_period = (double)interval.tv_sec + interval.tv_nsec / 1.0e9;
  NetworkUpdate ();
  net_receive_max = 0;
  net_transmit_max = 0;
  net_graph_scale = graph_scale;
  NetworkDrawBorder (win);
  net_canvas = CanvasCreate (win);
}

void
NetworkQuit ()
{
  list_delete (net_receive);
  list_delete (net_transmit);
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

static inline void
NetworkAddValue (List *l, uintmax_t v, bool full, uintmax_t *m)
{
  if (likely (full))
    list_rotate_left (l)->u = v;
  else
    list_push_back (l)->u = v;
  *m = 0;
  list_for_each (l, it)
    {
      if (it->u > *m)
        *m = it->u;
    }
}

void
NetworkUpdate ()
{
  const bool full = net_samples == net_max_samples;
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

  NetworkAddValue (net_receive, receive_period * net_period, full,
                   &net_receive_max);
  NetworkAddValue (net_transmit, transmit_period * net_period, full,
                   &net_transmit_max);

  if (unlikely (!full))
    ++net_samples;
}

static void
NetworkDrawGraph (List *values, uintmax_t max, unsigned y_off,
                  unsigned height, short color)
{
  double x = (net_max_samples - net_samples) * net_graph_scale;
  double y;
  const double scale = 1.0 / max * height;

  list_for_each (values, v)
    {
      x += net_graph_scale;
      y = y_off - (double)v->u * scale;

      if (likely (y >= 0.0))
        {
          CanvasDrawRect (net_canvas, x * 2.0, y_off * 4,
                          (x - net_graph_scale) * 2.0, y * 4.0, color);
        }
    }
}

static void
NetworkFlatline (unsigned y, short color)
{
  CanvasDrawLine (net_canvas, 0, y*4 - 1, net_canvas->width * 2, y*4 - 1, color);
}

void
NetworkDraw (WINDOW *win)
{
  CanvasClear (net_canvas);

  if (net_receive_max)
    NetworkDrawGraph (net_receive, net_receive_max, net_canvas->height / 2,
                      net_canvas->height / 4, C_NET_RECEIVE);
  else
    NetworkFlatline (net_canvas->height / 2, C_NET_RECEIVE);

  if (net_transmit_max)
    NetworkDrawGraph (net_transmit, net_transmit_max, net_canvas->height,
                      net_canvas->height / 4, C_NET_TRANSMIT);
  else
    NetworkFlatline (net_canvas->height, C_NET_TRANSMIT);

  CanvasDraw (net_canvas, win);

  wmove (win, 2, 3);
  waddstr (win, "Total RX:");
  wattron (win, COLOR_PAIR (C_NET_RECEIVE));
  FormatSize (win, net_receive_total, true);
  wattroff (win, COLOR_PAIR (C_NET_RECEIVE));
  wmove (win, 3, 3);
  waddstr (win, "RX/s:    ");
  wattron (win, COLOR_PAIR (C_NET_RECEIVE));
  FormatSize (win, net_receive->back->u, true);
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
  FormatSize (win, net_transmit->back->u, true);
  waddstr (win, "/s");
  wattroff (win, COLOR_PAIR (C_NET_TRANSMIT));
}

void
NetworkResize (WINDOW *win)
{
  wclear (win);

  CanvasResize (net_canvas, win);

  net_max_samples = MAX_SAMPLES (win, net_max_samples);
  list_shrink (net_receive, net_max_samples);
  list_shrink (net_transmit, net_max_samples);
  net_samples = net_receive->count;
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


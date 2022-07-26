#include "network.h"
#include "util.h"
#include "canvas/canvas.h"

extern struct timespec interval;

Widget net_widget = WIDGET(Network);

List *net_recieve;
List *net_transmit;
unsigned long net_recieve_total;
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
static uintmax_t net_recieve_max;
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
  net_recieve = list_create ();
  net_transmit = list_create ();
  net_recieve_total = 0;
  net_transmit_total = 0;
  net_max_samples = MAX_SAMPLES (win, graph_scale);
  net_samples = 0;
  NetworkGetInterfaces ();
  net_period = (double)interval.tv_sec + interval.tv_nsec / 1.0e9;
  NetworkUpdate ();
  net_recieve_max = 0;
  net_transmit_max = 0;
  net_graph_scale = graph_scale;
  DrawWindow (win, "Network");
  net_canvas = CanvasCreate (win);
}

void
NetworkQuit ()
{
  list_delete (net_recieve);
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
NetworkAddValue (List *l, uintmax_t v, bool shift, uintmax_t *m)
{
  if (likely (shift))
    {
      list_pop_front (l);
      *m = 0;
      list_for_each (l, i)
        {
          if (i->u > *m)
            *m = i->u;
        }
    }
  if (v > *m)
    *m = v;
  list_push_back (l)->u = v;
}

void
NetworkUpdate ()
{
  const bool shift = net_samples == net_max_samples;
  unsigned long prev_recieve = net_recieve_total;
  unsigned long prev_transmit = net_transmit_total;
  FILE *f;

  net_recieve_total = 0;
  net_transmit_total = 0;

  for (unsigned i = 0; i < net_interface_count; ++i)
    {
      *net_interfaces[i].rt_char = 'r';
      f = fopen (net_interfaces[i].name, "r");
      net_recieve_total += NetworkGetBytes (f);
      fclose (f);

      *net_interfaces[i].rt_char = 't';
      f = fopen (net_interfaces[i].name, "r");
      net_transmit_total += NetworkGetBytes (f);
      fclose (f);
    }

  double recieve_period = net_recieve_total - prev_recieve;
  double transmit_period = net_transmit_total - prev_transmit;

  NetworkAddValue (net_recieve, recieve_period * net_period, shift,
                   &net_recieve_max);
  NetworkAddValue (net_transmit, transmit_period * net_period, shift,
                   &net_transmit_max);

  if (unlikely (!shift))
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

void
NetworkDraw (WINDOW *win)
{
  CanvasClear (net_canvas);

  NetworkDrawGraph (net_recieve, net_recieve_max, net_canvas->height / 2,
                    net_canvas->height / 4, C_NET_RECIEVE);
  NetworkDrawGraph (net_transmit, net_transmit_max, net_canvas->height,
                    net_canvas->height / 4, C_NET_TRANSMIT);

  CanvasDraw (net_canvas, win);

  wmove (win, 2, 3);
  waddstr (win, "Total RX:");
  wattron (win, COLOR_PAIR (C_NET_RECIEVE));
  FormatSize (win, net_recieve_total, true);
  wattroff (win, COLOR_PAIR (C_NET_RECIEVE));
  wmove (win, 3, 3);
  waddstr (win, "RX/s:    ");
  wattron (win, COLOR_PAIR (C_NET_RECIEVE));
  FormatSize (win, net_recieve->back->u, true);
  waddstr (win, "/s");
  wattroff (win, COLOR_PAIR (C_NET_RECIEVE));

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
  DrawWindow (win, "Network");

  CanvasResize (net_canvas, win);

  net_max_samples = MAX_SAMPLES (win, net_max_samples);
  list_clear (net_recieve);
  list_clear (net_transmit);
  net_samples = 0;
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

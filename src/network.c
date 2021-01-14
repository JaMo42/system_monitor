#include "network.h"
#include "util.h"

extern struct timespec interval;

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
NetworkInit (unsigned max_samples)
{
  net_recieve = list_create ();
  net_transmit = list_create ();
  net_recieve_total = 0;
  net_transmit_total = 0;
  net_max_samples = max_samples;
  net_samples = 0;
  NetworkGetInterfaces ();
  net_period = (double)interval.tv_sec + interval.tv_nsec / 1.0e9;
}

void
NetworkQuit ()
{
  list_delete (net_recieve);
  list_delete (net_transmit);
  free (net_interfaces);
}

void
NetworkSetMaxSamples (unsigned s)
{
  net_max_samples = s;
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

  if (shift)
    {
      list_pop_front (net_recieve);
      list_pop_front (net_transmit);
    }
  list_push_back (net_recieve)->u = (uintmax_t)(recieve_period * net_period);
  list_push_back (net_transmit)->u = (uintmax_t)(transmit_period * net_period);
}

void
NetworkDraw (WINDOW *win)
{
  wmove (win, 2, 3);
  waddstr (win, "Total RX:");
  wattron (win, COLOR_PAIR (C_NET_RECIEVE));
  FormatSize (win, net_recieve_total, true);
  wattroff (win, COLOR_PAIR (C_NET_RECIEVE));

  waddstr (win, "  Total TX:");
  wattron (win, COLOR_PAIR (C_NET_TRANSMIT));
  FormatSize (win, net_transmit_total, true);
  wattroff (win, COLOR_PAIR (C_NET_TRANSMIT));

  wmove (win, 3, 3);
  waddstr (win, "RX/s:");
  wattron (win, COLOR_PAIR (C_NET_RECIEVE));
  FormatSize (win, net_recieve->back->u, true);
  waddstr (win, "/s");
  wattroff (win, COLOR_PAIR (C_NET_RECIEVE));

  wmove (win, 3, 22);
  waddstr (win, "TX/s:");
  wattron (win, COLOR_PAIR (C_NET_TRANSMIT));
  FormatSize (win, net_transmit->back->u, true);
  waddstr (win, "/s  ");
  wattroff (win, COLOR_PAIR (C_NET_TRANSMIT));
}


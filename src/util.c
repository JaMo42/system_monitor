#include "util.h"

const short C_BORDER = 214;
const short C_TITLE = 251;
const short C_CPU_AVG = 77;
const short C_CPU_GRAPHS[8] = {5, 4, 3, 2, 6, 7, 8, 9};
const short C_MEM_MAIN = 221;
const short C_MEM_SWAP = 209;
const short C_NET_RECIEVE = 5;
const short C_NET_TRANSMIT = 2;
const short C_PROC_HEADER = 16;
const short C_PROC_PROCESSES = 8;
const short C_PROC_CURSOR = 254;
const short C_PROC_HIGHLIGHT = 253;

void
DrawWindow (WINDOW *w, const char *title)
{
  wattron (w, COLOR_PAIR (C_BORDER));
  wborder (w, 0, 0, 0, 0, 0, 0, 0, 0);
  mvwaddch (w, 0, 1, '<');
  waddch (w, ' ');
  mvwaddch (w, 0, 4 + strlen (title), '>');
  wattroff (w, COLOR_PAIR (C_BORDER));
  wattron (w, COLOR_PAIR (C_TITLE));
  mvwaddstr (w, 0, 3, title);
  wattroff (w, COLOR_PAIR (C_TITLE));
  waddch (w, ' ');
}

void
DrawWindowInfo (WINDOW *w, const char *info)
{
  const int len = strlen (info);
  const int width = getmaxx (w);

  wattron (w, COLOR_PAIR (C_BORDER));
  mvwaddch (w, 0, width - 6 - len, '<');
  waddch (w, ' ');
  mvwaddch (w, 0, width - 4, ' ');
  waddch (w, '>');
  wattroff (w, COLOR_PAIR (C_BORDER));

  wattron (w, COLOR_PAIR (C_TITLE));
  mvwaddstr (w, 0, width - 4 - len, info);
  wattroff (w, COLOR_PAIR (C_TITLE));
}

void
DrawWindowInfo2 (WINDOW *w, const char *info)
{
  const int len = strlen (info);
  const int width = getmaxx (w);
  const int row = getmaxy (w) - 1;

  wattron (w, COLOR_PAIR (C_BORDER));
  mvwaddch (w, row, width - 6 - len, '<');
  waddch (w, ' ');
  mvwaddch (w, row, width - 4, ' ');
  waddch (w, '>');
  wattroff (w, COLOR_PAIR (C_TITLE));

  wattron (w, COLOR_PAIR (C_TITLE));
  mvwaddstr (w, row, width - 4 - len, info);
  wattroff (w, COLOR_PAIR (C_TITLE));
}

static const char *units[] = {" B", "KB", "MB", "GB", "TB", "PB"};

void
FormatSize (WINDOW *win, size_t size, bool pad)
{
  int order = 0;
  double s = size;
  while (s >= 1000.0 && order < 5)
    {
      ++order;
      s /= 1000.0;
    }
  wprintw (win, pad ? "% 6.1lf" : "%.1lf", s);
  waddstr (win, units[order]);
}

void
PrintN (WINDOW *win, int ch, unsigned n)
{
  while (--n)
    waddch (win, ch);
}

void
Strip (const char **s)
{
  while (isspace (**s))
    ++(*s);
}

int
TokenCount (const char *string, const char *delims)
{
  int count = 1, depth = 1;
  while (*string && depth)
    {
      if (isspace (*string) && !isspace (string[1]) && depth == 1)
        ++count;
      else if (*string == delims[0])
        ++depth;
      else if (*string == delims[1])
        --depth;
      ++string;
    }
  return count;
}

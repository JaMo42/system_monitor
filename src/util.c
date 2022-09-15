#include "util.h"

const short C_BORDER = 214;
const short C_TITLE = 251;
const short C_CPU_AVG = 77;
const short C_CPU_GRAPHS[8] = {5, 4, 3, 2, 6, 7, 8, 9};
const short C_MEM_MAIN = 221;
const short C_MEM_SWAP = 209;
const short C_NET_RECEIVE = 5;
const short C_NET_TRANSMIT = 2;
const short C_PROC_HEADER = 16;
const short C_PROC_PROCESSES = 8;
const short C_PROC_CURSOR = 254;
const short C_PROC_HIGHLIGHT = 253;
const short C_PROC_HIGH_PERCENT = 2;
const short C_PROC_BRANCHES = 243;
const short C_DISK_FREE = 248;
const short C_DISK_USED = 77;
const short C_DISK_ERROR = 2;

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

Size_Format
GetSizeFormat (size_t size)
{
  static const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
  int order = 0;
  double s = size;
  while (s >= KB && order < (int)countof (units))
    {
      ++order;
      s /= KB;
    }
  return (Size_Format) { .unit=units[order], .size=s };
}

void
FormatSize (WINDOW *win, size_t size, bool pad)
{
  Size_Format format = GetSizeFormat (size);
  wprintw (win, pad ? "% 6.1lf" : "%.1lf", format.size);
  wprintw (win, "%2s", format.unit);
}

void
PrintN (WINDOW *win, int ch, unsigned n)
{
  while (n--)
    waddch (win, ch);
}

char *
StringPush (char *buf, const char *s)
{
  while (*s)
    *buf++ = *s++;
  *buf = '\0';
  return buf;
}

static int push_syle_backup;

void
PushStyle (WINDOW *win, attr_t attributes, short color)
{
  attr_t attr;
  short current_color;
  wattr_get (win, &attr, &current_color, NULL);
  push_syle_backup = attr | COLOR_PAIR (current_color);
  wattrset (win, attributes | COLOR_PAIR (color));
}

void
PopStyle (WINDOW *win)
{
  wattrset (win, push_syle_backup);
}

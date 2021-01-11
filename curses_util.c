#include "curses_util.h"

const short C_BORDER = 214;
const short C_TITLE = 251;
const short C_ACCENT = 77;
const short C_GRAPH_TABLE[8] = {5, 4, 3, 2, 6, 7, 8, 9};

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

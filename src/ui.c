#include "ui.h"

Layout *
UICreateLayout (unsigned n, LayoutType type)
{
  Layout *l = (Layout *)malloc (sizeof (Layout));
  l->type = type;
  l->count = n;
  l->size = 1.0;
  l->elems = calloc (n, sizeof (Layout *));
  return l;
}

void
UIDeleteLayout (Layout *l)
{
  for (unsigned i = 0; i < l->count; ++i)
    {
      if (l->elems[i]->type == UI_WIDGET)
        {
          l->elems[i]->widget->Quit ();
          delwin (l->elems[i]->widget->win);
          free (l->elems[i]);
        }
      else
        UIDeleteLayout (l->elems[i]);
    }
  free (l->elems);
  free (l);
}

void
UIAddLayout (Layout *l, Layout *l2, unsigned i, float size)
{
  l->elems[i] = l2;
  l2->size = size;
}

void
UIAddWidget (Layout *l, Widget *w, unsigned i, float size)
{
  l->elems[i] = (Layout *)malloc (sizeof (Layout));
  l->elems[i]->type = UI_WIDGET;
  l->elems[i]->size = size;
  l->elems[i]->widget = w;
}

static void
UICreateWindows (Layout *l)
{
  for (unsigned i = 0; i < l->count; ++i)
    {
      if (l->elems[i]->type == UI_WIDGET)
        l->elems[i]->widget->win = newwin (1, 1, 0, 0);
      else
        UICreateWindows (l->elems[i]);
    }
}

static void UIResizeWindowsR (Layout *, unsigned, unsigned, unsigned,
                              unsigned);

static void
UIResizeWindowsC (Layout *l, unsigned x, unsigned y,
                  unsigned width, unsigned height)
{
  unsigned p = x;
  for (unsigned i = 0; i < l->count; ++i)
    {
      if (l->elems[i]->type == UI_WIDGET)
        {
          // Sometimes windows wouldn't move/resize properly and apparently
          // doing it twice fixes it.
          mvwin (l->elems[i]->widget->win, y, p);
          wresize (l->elems[i]->widget->win,
                   height, (unsigned)((float)width * l->elems[i]->size));
          mvwin (l->elems[i]->widget->win, y, p);
          wresize (l->elems[i]->widget->win,
                   height, (unsigned)((float)width * l->elems[i]->size));
        }
      else if (l->elems[i]->type == UI_ROWS)
        UIResizeWindowsR (l->elems[i], p, y,
                          (float)width * l->elems[i]->size, height);
      else
        UIResizeWindowsC (l->elems[i], p, y,
                          (float)width * l->elems[i]->size, height);
      p += (float)width * l->elems[i]->size;
    }
}

static void
UIResizeWindowsR (Layout *l, unsigned x, unsigned y,
                  unsigned width, unsigned height)
{
  unsigned p = y;
  for (unsigned i = 0; i < l->count; ++i)
    {
      if (l->elems[i]->type == UI_WIDGET)
        {
          // See above
          mvwin (l->elems[i]->widget->win, p, x);
          wresize (l->elems[i]->widget->win,
                   (unsigned)((float)height * l->elems[i]->size), width);
          mvwin (l->elems[i]->widget->win, p, x);
          wresize (l->elems[i]->widget->win,
                   (unsigned)((float)height * l->elems[i]->size), width);
        }
      else if (l->elems[i]->type == UI_ROWS)
        UIResizeWindowsR (l->elems[i], x, p,
                          width, (float)height * l->elems[i]->size);
      else
        UIResizeWindowsC (l->elems[i], x, p,
                          width, (float)height * l->elems[i]->size);
      p += (float)height * l->elems[i]->size;
    }
}

void
UIConstruct (Layout *l)
{
  struct winsize w;
  ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);
  UICreateWindows (l);
  if (l->type == UI_ROWS)
    UIResizeWindowsR (l, 0, 0, w.ws_col, w.ws_row);
  else
    UIResizeWindowsC (l, 0, 0, w.ws_col, w.ws_row);
}

static void
UIWidgetsResize (Layout *l)
{
  for (unsigned i = 0; i < l->count; ++i)
    {
      if (l->elems[i]->type == UI_WIDGET)
        l->elems[i]->widget->Resize (l->elems[i]->widget->win);
      else
        UIWidgetsResize (l->elems[i]);
    }
}

void
UIResize (Layout *l, unsigned width, unsigned height)
{
  if (l->type == UI_ROWS)
    UIResizeWindowsR (l, 0, 0, width, height);
  else
    UIResizeWindowsC (l, 0, 0, width, height);
  UIWidgetsResize (l);
}


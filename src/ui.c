#include "ui.h"

bool ui_too_small;

Layout *
UICreateLayout (unsigned n, LayoutType type)
{
  Layout *l = (Layout *)malloc (sizeof (Layout));
  l->type = type;
  l->count = n;
  l->size = 1.0;
  l->min_width = 0;
  l->min_height = 0;
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
  Layout *new = malloc (sizeof (Layout));
  new->type = UI_WIDGET;
  new->size = size;
  new->widget = w;
  w->MinSize (&new->min_width, &new->min_height);
  // Window borders
  new->min_height += 2;
  new->min_width += 2;
  l->elems[i] = new;
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
          wresize (l->elems[i]->widget->win,
                   height, (unsigned)((float)width * l->elems[i]->size));
          mvwin (l->elems[i]->widget->win, y, p);
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
          wresize (l->elems[i]->widget->win,
                   (unsigned)((float)height * l->elems[i]->size), width);
          mvwin (l->elems[i]->widget->win, p, x);
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
  if ((int)width < l->min_width || (int)height < l->min_height)
    {
      ui_too_small = true;
      return;
    }
  if (l->type == UI_ROWS)
    UIResizeWindowsR (l, 0, 0, width, height);
  else
    UIResizeWindowsC (l, 0, 0, width, height);
  UIWidgetsResize (l);
  ui_too_small = false;
}

void
UIGetMinSize (Layout *self)
{
  unsigned i;
  Layout *child;
  for (i = 0; i < self->count; ++i)
    {
      child = self->elems[i];
      if (child->type != UI_WIDGET)
        UIGetMinSize (child);
      if (self->type == UI_ROWS)
        {
          if (child->min_width > self->min_width)
            self->min_width = child->min_width;
          self->min_height += child->min_height;
        }
      else // UI_COLS
        {
          if (child->min_height > self->min_height)
            self->min_height = child->min_height;
          self->min_width += child->min_width;
        }
    }
}


#include "ui.h"

#define layout_for_each(l)                     \
  for (Layout **it = (l)->elems, *child = *it; \
       it != (l)->elems + 2;                   \
       child = *++it)

#define layout_sibling(l, c) \
  ((c) == (l)->elems[0] ? (l)->elems[1] : (l)->elems[0])

bool ui_too_small;

Layout *
UICreateLayout (LayoutType type)
{
  Layout *l = (Layout *)malloc (sizeof (Layout));
  *l = (Layout) {
    .type = type,
    .size = 1.0,
    .min_width = 0,
    .min_height = 0,
    .elems = {NULL, NULL}
  };
  return l;
}

void
UIDeleteLayout (Layout *l)
{
  layout_for_each (l)
    {
      if (child->type == UI_WIDGET)
        {
          child->widget->Quit ();
          delwin (child->widget->win);
          free (child);
        }
      else
        UIDeleteLayout (child);
    }
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
  layout_for_each (l)
    {
      if (child->type == UI_WIDGET)
        child->widget->win = newwin (1, 1, 0, 0);
      else
        UICreateWindows (child);
    }
}

static void UIResizeWindowsR (Layout *, unsigned, unsigned, unsigned,
                              unsigned);

static void
UIResizeWindowsC (Layout *l, unsigned x, unsigned y,
                  unsigned width, unsigned height)
{
  unsigned p = x;
  layout_for_each (l)
    {
      if (child->type == UI_WIDGET)
        {
          wresize (child->widget->win,
                   height, (unsigned)((float)width * child->size));
          mvwin (child->widget->win, y, p);
        }
      else if (child->type == UI_ROWS)
        UIResizeWindowsR (child, p, y,
                          (float)width * child->size, height);
      else
        UIResizeWindowsC (child, p, y,
                          (float)width * child->size, height);
      p += (float)width * child->size;
    }
}

static void
UIResizeWindowsR (Layout *l, unsigned x, unsigned y,
                  unsigned width, unsigned height)
{
  unsigned p = y;
  layout_for_each (l)
    {
      if (child->type == UI_WIDGET)
        {
          wresize (child->widget->win,
                   (unsigned)((float)height * child->size), width);
          mvwin (child->widget->win, p, x);
        }
      else if (child->type == UI_ROWS)
        UIResizeWindowsR (child, x, p,
                          width, (float)height * child->size);
      else
        UIResizeWindowsC (child, x, p,
                          width, (float)height * child->size);
      p += (float)height * child->size;
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
  layout_for_each (l)
    {
      if (child->type == UI_WIDGET)
        child->widget->Resize (child->widget->win);
      else
        UIWidgetsResize (child);
    }
}

static bool
UICheckSize (Layout *self, unsigned width, unsigned height)
{
  unsigned w, h;
  layout_for_each (self)
    {
      if (self->type == UI_ROWS)
        {
          w = width;
          h = (float)height * child->size;
        }
      else
        {
          w = (float)width * child->size;
          h = height;
        }
      if (child->type == UI_WIDGET)
        {
          if ((int)w < child->min_width || (int)h < child->min_height)
            return false;
        }
      else
        {
          if (!UICheckSize (child, w, h))
            return false;
        }
    }
  return true;
}

void
UIResize (Layout *l, unsigned width, unsigned height)
{
  if (!UICheckSize (l, width, height))
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
  layout_for_each (self)
    {
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


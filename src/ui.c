#include "ui.h"

#define layout_for_each(l)                     \
  for (Layout **it = (l)->elems, *child = *it; \
       it != (l)->elems + 2;                   \
       child = *++it)

#define layout_sibling(l, c) \
  ((c) == (l)->elems[0] ? (l)->elems[1] : (l)->elems[0])

#define max_priority(a, b) \
  ((a)->priority > (b)->priority ? (a) : (b))

#define cool_child(l)                                \
  ((l)->elems[0]->priority > (l)->elems[1]->priority \
   ? (l)->elems[0]                                   \
   : (l)->elems[1])

bool ui_too_small;

Layout *
UICreateLayout (LayoutType type, float percent_first)
{
  Layout *l = (Layout *)malloc (sizeof (Layout));
  *l = (Layout) {
    .type = type,
    .percent_first = percent_first,
    .min_width = 0,
    .min_height = 0,
    .priority = -1,
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
UIAddLayout (Layout *l, Layout *l2)
{
  l->elems[l->elems[0] != NULL] = l2;
}

void
UIAddWidget (Layout *l, Widget *w, int priority)
{
  Layout *new = malloc (sizeof (Layout));
  new->type = UI_WIDGET;
  new->widget = w;
  w->MinSize (&new->min_width, &new->min_height);
  // Window borders
  new->min_height += 2;
  new->min_width += 2;
  new->priority = priority;

  l->elems[l->elems[0] != NULL] = new;
  if (priority > l->priority)
    l->priority = priority;
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

static void
UIHide (Layout *self)
{
  if (self->type == UI_WIDGET)
    self->widget->hidden = true;
  else
    {
      UIHide (self->elems[0]);
      UIHide (self->elems[1]);
    }
}

static int
UIGetMin (Layout *self, bool height)
{
  Layout *child;
  if (self->type == UI_WIDGET)
    {
      if (self->widget->hidden)
        return 0;
      return height ? self->min_height : self->min_width;
    }
  else
    {
      child = cool_child (self);
      return height ? child->min_height : child->min_width;
    }
}

static void UIResizeWindowsR (Layout *, unsigned, unsigned, unsigned,
                              unsigned);

static void
UIResizeWindowsC (Layout *l, unsigned x, unsigned y,
                  unsigned width, const unsigned height)
{
  const int left_width = (float)width * l->percent_first;
  const int right_width = width - left_width;
  const int choice_width = width;  // for DO_RESIZE
  Layout *const left = l->elems[0];
  Layout *const right = l->elems[1];
  Layout *choice;

  if (left_width >= left->min_width && right_width >= right->min_width)
    {
#define DO_RESIZE(target_, x_)                                        \
      switch (target_->type)                                          \
        {                                                             \
        case UI_WIDGET:                                               \
          wresize (target_->widget->win, height, target_##_width);    \
          mvwin (target_->widget->win, y, x_);                        \
          target_->widget->hidden = false;                            \
          break;                                                      \
        case UI_ROWS:                                                 \
          UIResizeWindowsR (target_, x_, y, target_##_width, height); \
          break;                                                      \
        case UI_COLS:                                                 \
          UIResizeWindowsC (target_, x_, y, target_##_width, height); \
          break;                                                      \
        }
      DO_RESIZE (left, x);
      DO_RESIZE (right, x + left_width);
    }
  else
    {
      choice = max_priority (left, right);
      if ((int)width < UIGetMin (choice, false))
        choice = layout_sibling (l, choice);
      // no need to check again as we already ensured we can draw at least one
      // child in UICheckSize.
      DO_RESIZE (choice, x);
      UIHide (layout_sibling (l, choice));
    }
#undef DO_RESIZE
}

static void
UIResizeWindowsR (Layout *l, unsigned x, unsigned y,
                  const unsigned width, unsigned height)
{
  const int top_height = (float)height * l->percent_first;
  const int bottom_height = height - top_height;
  const int choice_height = height;  // for DO_RESIZE
  Layout *const top = l->elems[0];
  Layout *const bottom = l->elems[1];
  Layout *choice;

  if (top_height >= top->min_height && bottom_height >= bottom->min_height)
    {
#define DO_RESIZE(target_, y_)                                        \
      switch (target_->type)                                          \
        {                                                             \
        case UI_WIDGET:                                               \
          wresize (target_->widget->win, target_##_height, width);    \
          mvwin (target_->widget->win, y_, x);                        \
          target_->widget->hidden = false;                            \
          break;                                                      \
        case UI_ROWS:                                                 \
          UIResizeWindowsR (target_, x, y_, width, target_##_height); \
          break;                                                      \
        case UI_COLS:                                                 \
          UIResizeWindowsC (target_, x, y_, width, target_##_height); \
          break;                                                      \
        }
      DO_RESIZE (top, y);
      DO_RESIZE (bottom, y + top_height);
    }
  else
    {
      choice = max_priority (top, bottom);
      if ((int)width < UIGetMin (choice, true))
        choice = layout_sibling (l, choice);
      // no need to check again as we already ensured we can draw at least one
      // child in UICheckSize.
      DO_RESIZE (choice, x);
      UIHide (layout_sibling (l, choice));
    }
#undef DO_RESIZE
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

static bool UICheckSizeR (Layout *, unsigned, unsigned);

static bool
UICheckSizeC (Layout *self, unsigned width, unsigned height)
{
  const int left_width = (float)width * self->percent_first;
  const int right_width = width - left_width;
  Layout *const left = self->elems[0];
  Layout *const right = self->elems[1];
  Layout *choice;
  if (left_width >= left->min_width && right_width >= right->min_width)
    {
#define CHECK_CHILD(target_)                                      \
    if (target_->type == UI_ROWS                                  \
        && !UICheckSizeR (target_, target_##_width, height))      \
      return false;                                               \
    else if (target_->type == UI_COLS                             \
             && !UICheckSizeC (target_, target_##_width, height)) \
      return false
      CHECK_CHILD (left);
      CHECK_CHILD (right);
    }
  else
    {
      choice = max_priority (left, right);
      if ((int)width < UIGetMin (choice, false))
        {
          choice = layout_sibling (self, choice);
          if ((int)width < choice->min_width)
            return false;
        }
    }
  return true;
#undef CHECK_CHILD
}

static bool
UICheckSizeR (Layout *self, unsigned width, unsigned height)
{
  const int top_height = (float)height * self->percent_first;
  const int bottom_height = height - top_height;
  Layout *const top = self->elems[0];
  Layout *const bottom = self->elems[1];
  Layout *choice;
  if (top_height >= top->min_height && bottom_height >= bottom->min_height)
    {
#define CHECK_CHILD(target_)                                      \
    if (target_->type == UI_ROWS                                  \
        && !UICheckSizeR (target_, width, target_##_height))      \
      return false;                                               \
    else if (target_->type == UI_COLS                             \
             && !UICheckSizeC (target_, width, target_##_height)) \
      return false
      CHECK_CHILD (top);
      CHECK_CHILD (bottom);
    }
  else
    {
      choice = max_priority (top, bottom);
      if ((int)height < UIGetMin (choice, true))
        {
          choice = layout_sibling (self, choice);
          if ((int)height < choice->min_height)
            return false;
        }
    }
  return true;
#undef CHECK_CHILD
}

void
UIResize (Layout *l, unsigned width, unsigned height)
{
  if (!((l->type == UI_ROWS ? UICheckSizeR : UICheckSizeC) (l, width, height)))
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
      else
        {
          if (child->priority > self->priority)
            self->priority = child->priority;
        }
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


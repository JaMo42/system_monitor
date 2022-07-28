#include "ui.h"
#include "util.h"

#define layout_for_each(l)                    \
  for (Layout **it = (l)->elems,              \
             **end = it + 2,                  \
            *child = *it;                     \
       it != end;                             \
       ++it, child = (it < end ? *it : NULL))

#define layout_sibling(l, c) \
  ((c) == (l)->elems[0] ? (l)->elems[1] : (l)->elems[0])

#define max_priority(a, b) \
  ((a)->priority > (b)->priority ? (a) : (b))

#define cool_child(l)                                \
  ((l)->elems[0]->priority > (l)->elems[1]->priority \
   ? (l)->elems[0]                                   \
   : (l)->elems[1])

bool ui_too_small;
bool ui_strict_size = false;

static void UIResizeWindowsR (Layout *, unsigned, unsigned, unsigned,
                                 unsigned);
static bool UICheckSizeR (Layout *, unsigned, unsigned);
static inline Widget* UIResizeWindows (Layout *l, unsigned width,
                                       unsigned height);

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

static void
UIResizeWindowsC (Layout *l, unsigned x, unsigned y,
                  unsigned width, const unsigned height)
{
  int left_width = (float)width * l->percent_first;
  int right_width = width - left_width;
  const int choice_width = width;  // for DO_RESIZE
  Layout *const left = l->elems[0];
  Layout *const right = l->elems[1];
  Layout *choice;
  int diff;

  #define DO_RESIZE(target_, x_)                                    \
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

  if (left_width >= left->min_width && right_width >= right->min_width)
    {
      DO_RESIZE (left, x);
      DO_RESIZE (right, x + left_width);
    }
  else if (!ui_strict_size
           && left->min_width + right->min_width <= (int)width)
    {
      if (left->min_width > left_width)
        {
          diff = left->min_width - left_width;
          left_width += diff;
          right_width -= diff;
        }
      else
        {
          diff = right->min_width - right_width;
          left_width -= diff;
          right_width += diff;
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
  int top_height = (float)height * l->percent_first;
  int bottom_height = height - top_height;
  const int choice_height = height;  // for DO_RESIZE
  Layout *const top = l->elems[0];
  Layout *const bottom = l->elems[1];
  Layout *choice;
  int diff;

  #define DO_RESIZE(target_, y_)                                    \
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

  if (top_height >= top->min_height && bottom_height >= bottom->min_height)
    {
      DO_RESIZE (top, y);
      DO_RESIZE (bottom, y + top_height);
    }
  else if (!ui_strict_size
           && top->min_height + bottom->min_height <= (int)height)
    {
      if (top->min_height > top_height)
        {
          diff = top->min_height - top_height;
          top_height += diff;
          bottom_height -= diff;
        }
      else
        {
          diff = bottom->min_height - bottom_height;
          top_height -= diff;
          bottom_height += diff;
        }
      DO_RESIZE (top, y);
      DO_RESIZE (bottom, y + top_height);
    }
  else
    {
      choice = max_priority (top, bottom);
      if ((int)height < UIGetMin (choice, true))
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
  UICreateWindows (l);
  UIResizeWindows (l, COLS, LINES);
}

static void
UIWidgetsResize (Layout *l)
{
  layout_for_each (l)
    {
      if (child->type == UI_WIDGET)
        {
          child->widget->Resize (child->widget->win);
          child->widget->DrawBorder (child->widget->win);
        }
      else
        UIWidgetsResize (child);
    }
}

static bool
UICheckSizeC (Layout *self, unsigned width, unsigned height)
{
  const int left_width = (float)width * self->percent_first;
  const int right_width = width - left_width;
  Layout *const left = self->elems[0];
  Layout *const right = self->elems[1];
  Layout *choice;

  #define CHECK_CHILD(target_)                                    \
    if (target_->type == UI_ROWS                                  \
        && !UICheckSizeR (target_, target_##_width, height))      \
      return false;                                               \
    else if (target_->type == UI_COLS                             \
             && !UICheckSizeC (target_, target_##_width, height)) \
      return false

  if ((int)height < UIGetMin (left, true)
      || (int)height < UIGetMin (right, true))
    return false;
  else if (left_width >= left->min_width && right_width >= right->min_width)
    {
      CHECK_CHILD (left);
      CHECK_CHILD (right);
    }
  else
    {
      choice = max_priority (left, right);
      if ((int)width < UIGetMin (choice, false))
        {
          choice = layout_sibling (self, choice);
          if ((int)width < UIGetMin (choice, false))
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

  #define CHECK_CHILD(target_)                                    \
    if (target_->type == UI_ROWS                                  \
        && !UICheckSizeR (target_, width, target_##_height))      \
      return false;                                               \
    else if (target_->type == UI_COLS                             \
             && !UICheckSizeC (target_, width, target_##_height)) \
      return false

  if ((int)width < UIGetMin (top, false)
      || (int)width < UIGetMin (bottom, false))
    return false;
  if (top_height >= top->min_height && bottom_height >= bottom->min_height)
    {
      CHECK_CHILD (top);
      CHECK_CHILD (bottom);
    }
  else
    {
      choice = max_priority (top, bottom);
      if ((int)height < UIGetMin (choice, true))
        {
          choice = layout_sibling (self, choice);
          if ((int)height < UIGetMin (choice, true))
            return false;
        }
    }
  return true;
#undef CHECK_CHILD
}

void
UIBestFit (Layout *self, Layout **best_return, unsigned width, unsigned height)
{
  layout_for_each (self)
    {
      if (child->type == UI_WIDGET)
        {
          if (*best_return && (*best_return)->priority > child->priority)
            // We already have a fit and it has a higher priority
            continue;
          else if ((int)width >= child->min_width
                   && (int)height >= child->min_height)
            *best_return = child;
        }
      else
        UIBestFit (child, best_return, width, height);
    }
}

static inline Widget *
UIResizeWindows (Layout *l, unsigned width, unsigned height)
{
  Layout *show = NULL;
  if (!((l->type == UI_ROWS ? UICheckSizeR : UICheckSizeC) (l, width, height)))
    {
      UIBestFit (l, &show, width, height);
      if (show)
        {
          UIHide (l);
          wresize (show->widget->win, height, width);
          mvwin (show->widget->win, 0, 0);
          show->widget->hidden = false;
          ui_too_small = false;
          return show->widget;
        }
      else
        ui_too_small = true;
      return NULL;
    }
  if (l->type == UI_ROWS)
    UIResizeWindowsR (l, 0, 0, width, height);
  else
    UIResizeWindowsC (l, 0, 0, width, height);
  ui_too_small = false;
  return NULL;
}

void
UIResize (Layout *l, unsigned width, unsigned height)
{
  Widget *show_only;
  if ((show_only = UIResizeWindows (l, width, height)) != NULL)
    {
      show_only->Resize (show_only->win);
      show_only->DrawBorder (show_only->win);
    }
  else
    UIWidgetsResize (l);
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

Widget *
UIGetWidget (const char *name, Widget **begin, Widget **end, int *priority)
{
  Widget **it, *match = NULL;
  const char *name_end = name;
  size_t name_length = 0;
  while (isalnum (*name_end))
    ++name_end;
  name_length = name_end - name;
  if (*name_end != '[')
    {
priority_error:
      fprintf (stderr, "Missing priority for widget ‘%.*s’\n",
               (int)name_length, name);
      exit (1);
    }
  for (it = begin; it != end; ++it)
    {
      if (strlen ((*it)->name) >= name_length
          && strncmp (name, (*it)->name, name_length) == 0)
        {
          if (match)
            {
              fprintf (stderr,
                       "Ambiguous widget: ‘%.*s’ (matches ‘%s’ and ‘%s’)\n",
                       (int)name_length, name, match->name, (*it)->name);
              exit (1);
            }
          match = *it;
        }
    }
  if (match == NULL)
    {
      fprintf (stderr, "No matching widget: ‘%.*s’\n", (int)name_length, name);
      fputs ("Existing widgets are:\n", stderr);
      for (it = begin; it != end; ++it)
        fprintf (stderr, "  - ‘%s’\n", (*it)->name);
      exit (1);
    }
  *priority = strtol (name + name_length + 1, (char **)&name_end, 10);
  if (*name_end != ']')
    goto priority_error;
  return match;
}

Layout *
UIFromString (const char **source_ptr, Widget **widgets, int nwidgets)
{
  const char *word_end, *percentage_end, *source = *source_ptr;
  int n, priority, token_count;
  LayoutType type;
  float percent_first;
  Widget *widget, **widgets_end = widgets + nwidgets;
  Layout *self;

  #define Next()                     \
    do {                             \
      Strip (&source);               \
      word_end = source;             \
      while (*word_end               \
             && !isspace (*word_end) \
             && *word_end != ')')    \
        ++word_end;                  \
      n = word_end - source;         \
    } while (0)

  #define AddChild()                                                        \
    do {                                                                    \
      Next ();                                                              \
      if (*source == '(')                                                   \
        {                                                                   \
          *source_ptr = source;                                             \
          UIAddLayout (self, UIFromString (source_ptr, widgets, nwidgets)); \
          source = *source_ptr;                                             \
        }                                                                   \
      else                                                                  \
        {                                                                   \
          widget = UIGetWidget (source, widgets, widgets_end, &priority);   \
          UIAddWidget (self, widget, priority);                             \
          source += n;                                                      \
        }                                                                   \
    } while (0)

  // Skip beginning '('
  Strip (&source);
  ++source;

  // Token count
  token_count = TokenCount (source, "()");
  if (!(token_count == 3 || token_count == 4))
    {
      fputs ("Layout format should be ‘(rows|cols [percent_first] layout|widget"
             " layout|widget)’\n", stderr);
      exit (1);
    }

  // Type
  Next ();
  n = strchr (source, ' ') - source;
  if (strncmp (source, "rows", n) == 0
      || strncmp (source, "horizontal", n) == 0)
    type = UI_ROWS;
  else if (strncmp (source, "cols", n) == 0
           || strncmp (source, "columns", n) == 0
           || strncmp (source, "vertical", n) == 0)
    type = UI_COLS;
  else
    {
      fprintf (stderr, "Invalid layout type: %.*s\n", n, source);
      fputs ("Valid types are:\n", stderr);
      fputs ("  - ‘rows’, ‘horizontal’\n", stderr);
      fputs ("  - ‘cols’, ‘columns’, ‘vertical’\n", stderr);
      exit (1);
    }
  source += n;

  // Percentage size of first child
  if (token_count == 4)
    {
      Next ();
      if (!isdigit (*source))
        {
          fputs ("Relative size of first child must be a positive number\n",
                 stderr);
          exit (1);
        }
      percent_first = ((float)strtoul (source, (char **)&percentage_end, 10)
                       / 100.0f);
      if (*percentage_end != '%' || percentage_end+1 != word_end)
        {
          fputs ("Relative size of first child must be in format ‘<percent>%’.",
                stderr);
          exit (1);
        }
      if (percent_first > 1.0f)
        percent_first = 1.0f;
      source += n;
    }
  else
    percent_first = 0.5f;

  // Layout
  self = UICreateLayout (type, percent_first);

  // Children
  AddChild ();
  AddChild ();

  // SKip ')'
  Strip (&source);
  ++source;
  *source_ptr = source;

  return self;
#undef AddChild
#undef Next
}

Widget **
UICollectWidgets (Layout *l, Widget **widgets_out)
{
  layout_for_each (l)
    {
      if (child->type == UI_WIDGET)
        {
          child->widget->exists = true;
          *widgets_out++ = child->widget;
        }
      else
        widgets_out = UICollectWidgets (child, widgets_out);
    }
  return widgets_out;
}

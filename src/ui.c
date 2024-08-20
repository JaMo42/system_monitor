#include "ui.h"
#include "util.h"
#include "sm.h"

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

typedef void (*UI_Visitor) (const Layout *, void *);

bool ui_too_small;
bool ui_strict_size = false;

/**
 * calls `visitor (layout, data)` with each leaf node in the given layout tree.
 * @param self the layout tree
 * @param data any data that's simply passed to the visitor function
 * @param visitor function to call with each leaf layout.
 */
static void
UIForEachWidget (const Layout *self, void *data, UI_Visitor visitor)
{
  if (self->type == UI_WIDGET)
    visitor (self, data);
  else
    {
      UIForEachWidget (self->elems[0], data, visitor);
      UIForEachWidget (self->elems[1], data, visitor);
    }
}

Layout *
UICreateLayout (LayoutType type, float percent_first)
{
  Layout *l = (Layout *)malloc (sizeof (Layout));
  *l = (Layout) {
    .type = type,
    .percent_first = percent_first,
    .min_width = 0,
    .min_height = 0,
    .x = 0,
    .y = 0,
    .width = 0,
    .height = 0,
    .priority = -1,
    .elems = {NULL, NULL}
  };
  return l;
}

void
UIDeleteLayout (Layout *self)
{
  if (self->type == UI_WIDGET)
    {
      self->widget->Quit ();
      delwin (self->widget->win);
      free (self);
      return;
    }
  layout_for_each (self)
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
  free (self);
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
  new->min_width += 2;
  new->min_height += 2;

  new->priority = priority;

  l->elems[l->elems[0] != NULL] = new;
  if (priority > l->priority)
    l->priority = priority;
}

static void
UIHideVisitor(const Layout *w, void *_)
{
  (void)_;
  w->widget->hidden = true;
}

/** hides all widgets in the given layout. */
static void
UIHide (const Layout *self)
{
  UIForEachWidget (self, NULL, UIHideVisitor);
}

/**
 * gets the minimum width/height of the given layout.
 *
 * If the given layout is a widget, the minimum width/height of the widget is
 * returned (0 if it's hidden). If it's a layout the minimum size of the
 * child with the higher priority is returned.
 *
 * @param self the layout
 * @param height whether to get the height or width
 */
static int
UIGetMin (const Layout *self, bool height)
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

/**
 * set the position and size of a layout containing a widget.
 */
static void
UISetWidgetDimensions (Layout *self, int x, int y, int w, int h)
{
  wresize (self->widget->win, h, w);
  mvwin (self->widget->win, y, x);
  self->x = x;
  self->y = y;
  self->width = w;
  self->height = h;
}

/**
 * gets the width/height for the children of the given layout.
 * @param self the layout
 * @param total the width/height available
 * @param percent_first the relative size the top/left child should have
 * @param height whether to get the height or width
 * @param the width/height for the children of the layout; if one of the values
 *            is -1 the child should be hidden
 */
static inline struct UI_Get_Size_Return {
  int first, second;
} UIGetSize (const Layout *self, int total, float percent_first, bool height)
{
  const Layout *const first = self->elems[0];
  const Layout *const second = self->elems[1];
  const int first_min = height ? first->min_height : first->min_width;
  const int second_min = height ? second->min_height : second->min_width;
  bool first_fixed = first->type == UI_WIDGET && first->widget->fixed_size;
  bool second_fixed = second->type == UI_WIDGET && second->widget->fixed_size;
  int first_size = (float)total * percent_first;
  int second_size = total - first_size;
  int diff;
  const Layout *choice;

  if (first_fixed && second_fixed)
    {
      /* we don't want gaps so only one of two adjacent nodes can have a
         fixed size. */
      if (first->priority > second->priority)
        second_fixed = false;
      else
        first_fixed = false;
    }

  if (first_size >= first_min && second_size >= second_min)
    ;
  else if (!ui_strict_size && first_min + second_min <= total)
    {
      if (first_min > first_size)
        {
          diff = first_min - first_size;
          first_size += diff;
          second_size -= diff;
        }
      else
        {
          diff = second_min - second_size;
          first_size -= diff;
          second_size += diff;
        }
    }
  else
    {
      choice = max_priority (first, second);
      if (total < UIGetMin (choice, height))
        choice = layout_sibling (self, choice);
      if (choice == first)
        {
          first_size = total;
          second_size = -1;
        }
      else
        {
          first_size = -1;
          second_size = total;
        }
    }

  if (first_fixed && first_min + second_min <= total)
    {
      first_size = first_min;
      second_size = total - first_min;
    }
  else if (second_fixed && second_min + first_min <= total)
    {
      first_size = total - second_min;
      second_size = second_min;
    }

  return (struct UI_Get_Size_Return) {
    .first=first_size,
    .second=second_size
  };
}

/** recursively repositions and resizes, or hides the windows in the given
    layout tree according to their preferred size and priority. */
static void
UIResizeLayout (Layout *self, int x, int y, int width, int height)
{
  const bool get_height = self->type == UI_ROWS;
  const int total = get_height ? height : width;
  struct UI_Get_Size_Return sizes
    = UIGetSize (self, total, self->percent_first, get_height);
  struct Geometry {
    int x, y, w, h;
  } geometries[2];

  if (sizes.first == -1)
    {
      geometries[0].x = -1;
      geometries[1] = (struct Geometry) {
        .x = x, .y = y, .w = width, .h = height
      };
    }
  else if (sizes.second == -1)
    {
      geometries[0] = (struct Geometry) {
        .x = x, .y = y, .w = width, .h = height
      };
      geometries[1].x = -1;
    }
  else
    {
      if (get_height)
        {
          // Rows
          geometries[0] = (struct Geometry) {
            .x = x, .y = y, .w = width, .h = sizes.first
          };
          geometries[1] = (struct Geometry) {
            .x = x, .y = y + sizes.first, .w = width, .h = sizes.second
          };
        }
      else
        {
          // Columns
          geometries[0] = (struct Geometry) {
            .x = x, .y = y, .w = sizes.first, .h = height
          };
          geometries[1] = (struct Geometry) {
            .x = x + sizes.first, .y = y, .w = sizes.second, .h = height
          };
        }
    }

  struct Geometry *g = geometries;
  layout_for_each (self)
    {
      if (g->x == -1)
        UIHide (child);
      else
        {
          if (child->type == UI_WIDGET)
            {
              UISetWidgetDimensions (child, g->x, g->y, g->w, g->h);
              child->widget->hidden = false;
            }
          else
            UIResizeLayout (child, g->x, g->y, g->w, g->h);
        }
      ++g;
    }
  self->x = x;
  self->y = y;
  self->width = width;
  self->height = height;
}

static bool
UICheckSize (const Layout *self, int width, int height)
{
  const bool get_height = self->type == UI_ROWS;
  const int total = get_height ? height : width;
  const Layout *const first = self->elems[0];
  const Layout *const second = self->elems[1];
  const int other_size = get_height ? width : height;
  struct UI_Get_Size_Return sizes;
  const Layout *choice;

  if (other_size < UIGetMin (first, !get_height)
      || other_size < UIGetMin (second, !get_height))
    return false;
  sizes = UIGetSize (self, get_height ? height : width,
                     self->percent_first, get_height);
  if (sizes.first == -1 || sizes.second == -1)
    {
      // Need to re-check since `UIGetSize` assumes at least one fits.
      choice = max_priority (first, second);
      if (total < UIGetMin (choice, get_height))
        {
          choice = layout_sibling (self, choice);
          if (total < UIGetMin (self, choice))
            return false;
        }
    }
  else if (first->type != UI_WIDGET
           && !UICheckSize (first,
                            get_height ? width : sizes.first,
                            get_height ? sizes.first : height))
    return false;
  else if (second->type != UI_WIDGET
           && !UICheckSize (second,
                            get_height ? width : sizes.second,
                            get_height ? sizes.second : height))
    return false;
  return true;
}

typedef struct {
  const Layout *best;
  const int width;
  const int height;
} UIBestFitData;

static void
UIBestFitVisitor(const Layout *w, void *data_p)
{
  UIBestFitData *data = data_p;
  if (data->best && data->best->priority > w->priority)
    return;
  if (data->width >= w->min_width && data->height >= w->min_height)
    data->best = w;
}

void
UIBestFit (Layout *self, Layout **best_return, int width, int height)
{
  UIBestFitData data = {
    .best = NULL,
    .width = width,
    .height = height
  };
  UIForEachWidget (self, &data, UIBestFitVisitor);
  *best_return = (Layout *)data.best;
}

static inline Widget *
UIResizeWindows (Layout *l, int width, int height)
{
  Layout *show = NULL;
  if (l->type == UI_WIDGET)
    {
      /* single-widget layout */
      ui_too_small = width < l->min_width || height < l->min_height;
      if (!ui_too_small)
        {
          UISetWidgetDimensions (l, 0, 0, width, height);
        }
      return l->widget;
    }
  if (!UICheckSize (l, width, height))
    {
      /* cannot fit tree, look for a single widget that fits */
      UIBestFit (l, &show, width, height);
      if (show)
        {
          UIHide (l);
          UISetWidgetDimensions (show, 0, 0, width, height);
          show->widget->hidden = false;
          ui_too_small = false;
          return show->widget;
        }
      else
        ui_too_small = true;
      return NULL;
    }
  /* show the layout normally */
  UIResizeLayout (l, 0, 0, width, height);
  ui_too_small = false;
  return NULL;
}

static void
UIConstructVisitor(const Layout *w, void *_)
{
  (void)_;
  w->widget->win = newwin(1, 1, 0, 0);
}

void
UIConstruct (Layout *self)
{
  if (self->type == UI_WIDGET)
    {
      self->widget->win = newwin (LINES, COLS, 0, 0);
      self->widget->hidden = false;
    }
  else
    {
      UIForEachWidget (self, NULL, UIConstructVisitor);
      UIResizeWindows (self, COLS, LINES);
    }
}


static void
UIResizeVisitor(const Layout *w, void *_)
{
  (void)_;
  if (!w->widget->hidden)
    {
      w->widget->Resize (w->widget->win);
      w->widget->DrawBorder (w->widget->win);
    }
}

void
UIResize (Layout *self, unsigned width, unsigned height)
{
  Widget *show_only;
  if ((show_only = UIResizeWindows (self, width, height)) != NULL)
    {
      show_only->Resize (show_only->win);
      show_only->DrawBorder (show_only->win);
    }
  else
    UIForEachWidget (self, NULL, UIResizeVisitor);
}

void
UIGetMinSize (Layout *self)
{
  if (self->type == UI_WIDGET)
    return;
  layout_for_each (self)
    {
      if (child->type != UI_WIDGET)
        {
          child->min_width = 0;
          child->min_height = 0;
          UIGetMinSize (child);
        }
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

static void
UIUpdateSizeInfoVisitor(const Layout *w, void *changed)
{
  const int width = w->min_width, height = w->min_height;
  const int fixed = w->type == UI_WIDGET ? w->widget->fixed_size : FIXED_SIZE_NO;
  Layout *mut = (Layout *)w;

  w->widget->MinSize (&mut->min_width, &mut->min_height);
  mut->min_width += 2;
  mut->min_height += 2;
  if (w->type == UI_WIDGET && w->widget->fixed_size == FIXED_SIZE_SET)
    w->widget->fixed_size = FIXED_SIZE_YES;

  if (w->min_width != width || w->min_height != height
      || fixed != (w->type == UI_WIDGET ? w->widget->fixed_size : FIXED_SIZE_NO))
    *(bool *)changed = true;
}

void
UIUpdateSizeInfo (Layout *self, bool force_update)
{
  bool changed = force_update || self->type == UI_WIDGET;
  UIForEachWidget (self, &changed, UIUpdateSizeInfoVisitor);
  if (changed)
    {
      UIGetMinSize (self);
      HandleResize ();
    }
}

static void
UICollectWidgetsVisitor(const Layout *w, void *data)
{
  w->widget->exists = true;
  *(*(Widget ***)data)++ = w->widget;
}

void
UICollectWidgets (const Layout *self, Widget **widgets_out)
{
  UIForEachWidget (self, &widgets_out, UICollectWidgetsVisitor);
}

Layout *
UIFindWidgetContaining (Layout *self, int x, int y)
{
  if (self->type == UI_WIDGET)
    return self;
  if (InRange (x, self->x, self->x + self->elems[0]->width)
      && InRange (y, self->y, self->y + self->elems[0]->height))
    return UIFindWidgetContaining (self->elems[0], x, y);
  else
    return UIFindWidgetContaining (self->elems[1], x, y);
}

bool UIIsVisible(Layout *self) {
  if (self->type == UI_WIDGET)
    return !self->widget->hidden;
  return UIIsVisible(self->elems[0]) || UIIsVisible(self->elems[1]);
}

Layout* UIBottomRightVisibleNode(Layout* self, Layout* prev) {
  if (self->type == UI_WIDGET)
    return self->widget->hidden ? prev : self;
  bool second_is_visible = UIIsVisible(self->elems[1]);
  return UIBottomRightVisibleNode(self->elems[second_is_visible], self);
}

Widget* UIBottomRightVisibleWidget(Layout* self) {
  return UIBottomRightVisibleNode(self, NULL)->widget;
}

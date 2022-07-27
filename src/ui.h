#pragma once
#include "stdafx.h"
#include "widget.h"

typedef enum
{
  UI_ROWS,
  UI_COLS,
  UI_WIDGET
} LayoutType;

typedef struct Layout
{
  LayoutType type;
  float percent_first;
  int min_width;
  int min_height;
  int priority;
  union
    {
      struct Layout *elems[2];
      Widget *widget;
    };
} Layout;

Layout* UICreateLayout (LayoutType type, float percent_first);
void    UIDeleteLayout (Layout *l);
void    UIAddLayout (Layout *l, Layout *l2);
void    UIAddWidget (Layout *l, Widget *w, int priority);
void    UIConstruct (Layout *l);
void    UIResize (Layout *l, unsigned width, unsigned height);
void    UIGetMinSize (Layout *l);
Layout* UIFromString (const char **source, Widget **widgets, int nwidgets);

extern bool ui_too_small;
extern bool ui_strict_size;

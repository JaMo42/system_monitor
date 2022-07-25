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
  float size;
  int min_width;
  int min_height;
  union
    {
      struct Layout *elems[2];
      Widget *widget;
    };
} Layout;

Layout *UICreateLayout (LayoutType type);
void    UIDeleteLayout (Layout *l);
void    UIAddLayout (Layout *l, Layout *l2, unsigned i, float size);
void    UIAddWidget (Layout *l, Widget *w, unsigned i, float size);
void    UIConstruct (Layout *l);
void    UIResize (Layout *l, unsigned width, unsigned height);
void    UIGetMinSize (Layout *l);

extern bool ui_too_small;


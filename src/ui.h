#pragma once
#include "stdafx.h"
#include "widget.h"

typedef enum
{
  UI_ROWS,
  UI_COLS,
  UI_WIDGET
} LayoutType;

/* Is actually a layout or a widget but don't tell anyone. */
typedef struct Layout
{
  LayoutType type;
  unsigned count;
  float size;
  union
    {
      struct Layout **elems;
      Widget *widget;
    };
} Layout;

Layout *UICreateLayout (unsigned n, LayoutType type);
void    UIDeleteLayout (Layout *l);
void    UIAddLayout (Layout *l, Layout *l2, unsigned i, float size);
void    UIAddWidget (Layout *l, Widget *w, unsigned i, float size);

void UIConstruct (Layout *l);
void UIResize (Layout *l, unsigned width, unsigned height);

/*void UIUpdate (Layout *l);
void UIDraw (Layout *l);*/


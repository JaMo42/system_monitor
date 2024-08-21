#pragma once
#include "stdafx.h"
#include "widget.h"

typedef enum { UI_ROWS, UI_COLS, UI_WIDGET } LayoutType;

typedef struct Layout {
    LayoutType type;
    float percent_first;
    int min_width;
    int min_height;
    int x, y;
    int width, height;
    int priority;
    union {
        struct Layout *elems[2];
        Widget *widget;
    };
} Layout;

/** whether there are no widgets that fit the current screen size. */
extern bool ui_too_small;

/** whether the relative size may be adjusted to fir both children. */
extern bool ui_strict_size;

/** creates a new node. */
Layout *UICreateLayout(LayoutType type, float percent_first);

/** deletes the layout tree, quitting and deleting all its widgets. */
void UIDeleteLayout(Layout *self);

/** pushes a layout child to the given node. */
void UIAddLayout(Layout *self, Layout *l2);

/** pushes a widget child to the given node. */
void UIAddWidget(Layout *self, Widget *w, int priority);

/** creates the windows for the widgets in the given layout. */
void UIConstruct(Layout *self);

/** resizes or hides the widgets in the given layout. */
void UIResize(Layout *self, unsigned width, unsigned height);

/** sets the appropriate minimum size and priority for non-widget nodes. */
void UIGetMinSize(Layout *self);

/** refreshes all widget size information and triggers a resize.  By default
    the resize is only triggered if anything changed, it can be forced using the
    `force_update` parameter. */
void UIUpdateSizeInfo(Layout *self, bool force_update);

/** gets all widgets present in the given layout and marks them as existent. */
void UICollectWidgets(const Layout *self, Widget **widgets_out);

/** gets the widget containing the given point. */
Layout *UIFindWidgetContaining(Layout *self, int x, int y);

/** gets the widget in the bottom right corner */
Widget *UIBottomRightVisibleWidget(Layout *self);

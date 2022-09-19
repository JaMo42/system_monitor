#pragma once
#include "stdafx.h"
#include "input.h"

enum
{
  FIXED_SIZE_NO = false,
  FIXED_SIZE_YES = true,
  FIXED_SIZE_SET = false + true + 1
};

typedef struct Widget
{
  const char *const name;
  WINDOW *win;
  int fixed_size;
  bool hidden;
  bool exists;
  void (*Init) (WINDOW *win, unsigned graph_scale);
  void (*Quit) ();
  void (*Update) ();
  void (*Draw) (WINDOW *win);
  void (*Resize) (WINDOW *win);
  void (*MinSize) (int *width_return, int *height_return);
  bool (*HandleInput) (int key);
  void (*HandleMouse) (Mouse_Event *event);
  void (*DrawBorder) (WINDOW *win);
} Widget;

#define WIDGET(ident, name_)           \
  (Widget) {                           \
    .name = ident,                     \
    .win = NULL,                       \
    .hidden = false,                   \
    .exists = false,                   \
    .fixed_size = FIXED_SIZE_NO,       \
    .Init = name_##Init,               \
    .Quit = name_##Quit,               \
    .Update = name_##Update,           \
    .Draw = name_##Draw,               \
    .Resize = name_##Resize,           \
    .MinSize = name_##MinSize,         \
    .HandleInput = name_##HandleInput, \
    .HandleMouse = name_##HandleMouse, \
    .DrawBorder = name_##DrawBorder    \
  }

#define IgnoreInput(name_)          \
  bool name_##HandleInput (int key) \
  {                                 \
    (void)key;                      \
    return false;                   \
  }

#define IgnoreMouse(name_)                     \
  void name_##HandleMouse (Mouse_Event *event) \
  {                                            \
    (void)event;                               \
  }

static inline void
WidgetFixedSize (Widget *widget, bool yay_or_nay)
{
  widget->fixed_size = yay_or_nay ? FIXED_SIZE_SET : FIXED_SIZE_NO;
}

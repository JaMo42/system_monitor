#pragma once
#include "stdafx.h"

typedef struct Widget
{
  const char *const name;
  WINDOW *win;
  bool hidden;
  void (*Init) (WINDOW *win, unsigned graph_scale);
  void (*Quit) ();
  void (*Update) ();
  void (*Draw) (WINDOW *win);
  void (*Resize) (WINDOW *win);
  void (*MinSize) (int *width_return, int *height_return);
} Widget;

#define WIDGET(ident, name_)  \
  (Widget) {                  \
    .name = ident,            \
    .win = NULL,              \
    .hidden = false,          \
    .Init = name_##Init,      \
    .Quit = name_##Quit,      \
    .Update = name_##Update,  \
    .Draw = name_##Draw,      \
    .Resize = name_##Resize,  \
    .MinSize = name_##MinSize \
  }

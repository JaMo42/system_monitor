#pragma once
#include "stdafx.h"

typedef struct Widget
{
  WINDOW *win;
  void (*Init) (WINDOW *win, unsigned graph_scale);
  void (*Quit) ();
  void (*Update) ();
  void (*Draw) (WINDOW *win);
  void (*Resize) (WINDOW *win);
} Widget;

#define WIDGET(name)\
  (Widget) {\
    .Init = name##Init, \
    .Quit = name##Quit, \
    .Update = name##Update, \
    .Draw = name##Draw, \
    .Resize = name##Resize\
  }


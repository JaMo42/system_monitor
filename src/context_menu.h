#pragma once
#include "stdafx.h"

typedef struct {
  const char **names;
  void (**functions) ();
  int count;
  int width;
} Context_Menu;

Context_Menu ContextMenuCreate (const char **names, void (**functions) ());

void ContextMenuShow (Context_Menu *menu, int x, int y);

#pragma once
#include "stdafx.h"

typedef struct {
  const char **names;
  int count;
  int width;
} Context_Menu;

Context_Menu ContextMenuCreate (const char **names, int count);

int ContextMenuShow (Context_Menu *menu, int x, int y);

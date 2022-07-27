#pragma once
#include "stdafx.h"

extern pthread_mutex_t draw_mutex;
extern bool (*HandleInput) (int);

bool MainHandleInput (int key);

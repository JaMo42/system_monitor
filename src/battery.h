#pragma once
#include "stdafx.h"
#include "widget.h"

extern const char *battery_battery;
extern bool battery_slim;
extern const char *battery_show_status;

extern Widget battery_widget;

void BatteryInit(WINDOW * win);
void BatteryQuit();
void BatteryUpdate();
void BatteryDraw(WINDOW * win);
void BatteryResize(WINDOW * win);
void BatteryMinSize(int *width_return, int *height_return);
void BatteryDrawBorder(WINDOW * win);

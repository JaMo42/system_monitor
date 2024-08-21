#include "battery.h"
#include "util.h"

IgnoreMouse(Battery);
IgnoreInput(Battery);
Widget battery_widget = WIDGET("battery", Battery);

const char *battery_battery = "BAT0";
bool battery_slim = false;
const char *battery_show_status = "Charging,Not charging";

static char *battery_capacity_path;
static char *battery_status_path;

static char battery_capacity[4];
static char battery_status[13]; // The longest status is "Not charging"
static unsigned battery_last_capacity = 0;

void BatteryInit(WINDOW *win) {
    strcpy(battery_capacity, "50");
    battery_capacity_path = Format("/sys/class/power_supply/%s/capacity", battery_battery);
    battery_status_path = Format("/sys/class/power_supply/%s/status", battery_battery);
    BatteryDrawBorder(win);
}

void BatteryQuit() {
    free(battery_capacity_path);
    free(battery_status_path);
}

static bool ShowStatus() {
    const size_t n = strlen(battery_status);
    const char *s = battery_show_status;
    do {
        if (strncasecmp(s, battery_status, n) == 0) {
            return true;
        }
        s = strchr(s, ',');
        if (s) {
            ++s;
        }
    } while (s);
    return false;
}

void BatteryUpdate() {
    strncpy(battery_capacity, ReadSmallFile(battery_capacity_path, true), sizeof(battery_capacity) - 1);
    strncpy(battery_status, ReadSmallFile(battery_status_path, true), sizeof(battery_status) - 1);
    if (!ShowStatus()) {
        battery_status[0] = '\0';
    }
}

void BatteryDraw(WINDOW *win) {
    const unsigned capacity = atoi(battery_capacity);
    if (capacity == battery_last_capacity) {
        return;
    }
    const int height = getmaxy(win) - 2;
    const int width = getmaxx(win) - 2;
    const int fill = width * capacity / 100;
    for (int i = 1; i <= height; ++i) {
        wmove(win, i, 1);
        wattr_set(win, A_NORMAL, theme->battery_fill, NULL);
        PrintN(win, ' ', fill);
        if (battery_last_capacity > capacity) {
            wattr_set(win, A_NORMAL, A_NORMAL, NULL);
            PrintN(win, ' ', width - fill);
        }
    }
    char label[strlen("100% (Not charing)")];
    snprintf(label, sizeof(label), "%u%%", capacity);
    if (battery_status[0]
        && (strlen(label) + 3 + strlen(battery_status)) <= (unsigned)width) {
        snprintf(label, sizeof(label), "%u%% (%s)", capacity, battery_status);
    }
    int x = 1 + (width - strlen(label) + 1) / 2;
    wmove(win, 1 + height / 2, x);
    for (char *c = label; *c; ++c) {
        if (x++ <= fill) {
            wattr_set(win, A_NORMAL, theme->battery_fill, NULL);
        } else {
            wattr_set(win, A_NORMAL, A_NORMAL, NULL);
        }
        waddch(win, *c);
    }
    battery_last_capacity = capacity;
}

void BatteryResize(WINDOW *win) {
    battery_last_capacity = 0;
    BatteryDrawBorder(win);
}

void BatteryMinSize(int *width_return, int *height_return) {
    *width_return = strlen(battery_battery) + 2 + 2;
    *height_return = battery_slim ? 1 : 3;
}

void BatteryDrawBorder(WINDOW *win) {
    DrawWindow(win, "Battery");
}

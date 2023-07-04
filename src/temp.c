#include "temp.h"
#include "util.h"
#include "ps/util.h"

IgnoreInput(Temp);
IgnoreMouse(Temp);
Widget temp_widget = WIDGET("temperature", Temp);

const char *temp_filter = NULL;
bool temp_show_average = false;

typedef struct {
    char *temp_path;
    char *type;
    // size is determined by the compiler warning after using a very small value
    char temp[15];
    int number; // for sorting
} ThermalZone;

static bool did_filter = false;
static VECTOR(ThermalZone) zones = NULL;
static char average_temp[15] = "";

static bool TempFilter(const char *entry_name, VECTOR(char *) filter) {
    if (filter == NULL) {
        return true;
    }
    vector_for_each(filter, f) {
        if (strcmp(entry_name, *f) == 0) {
            return true;
        }
    }
    return false;
}

static int TempCompareZone(const void *a, const void *b) {
    return ((ThermalZone *)a)->number - ((ThermalZone *)b)->number;
}

static void TempDiscover(VECTOR(char *) filter) {
    const char *BASE_PATH = "/sys/class/thermal";
    DIR *dir = opendir(BASE_PATH);
    struct dirent *entry = NULL;
    const char CHECK[] = "thermal_zone";
    while ((entry = readdir(dir))) {
        if (strncmp(entry->d_name, CHECK, sizeof(CHECK) - 1) == 0) {
            char *path = Format("%s/%s/type", BASE_PATH, entry->d_name);
            char *type = ReadSmallFile(path, true);
            if (TempFilter(entry->d_name, filter) || TempFilter(type, filter)) {
                memcpy(path + strlen(path) - 4, "temp", 4);
                vector_emplace_back(
                    zones,
                    .temp_path = path,
                    .type = strdup(type),
                    .temp = "",
                    .number = atoi(entry->d_name + 12)
                );
            } else {
                did_filter = true;
            }
        }
    }
    if (zones) {
        qsort(zones, vector_size(zones), sizeof(*zones), TempCompareZone);
    }
}

static VECTOR(char *) TempGetFilter(const char *filter_string) {
    if (filter_string == NULL) {
        return NULL;
    }
    VECTOR(char *) filter = NULL;
    const char *start = filter_string;
    const char *p = NULL;
    for (p = filter_string; *p; ++p) {
        if (*p == ';' || *p == ',') {
            ptrdiff_t len = p - start;
            if (len == 0) {
                start = p + 1;
                continue;
            }
            vector_push(filter, strndup(start, len));
            start = p + 1;
        }
    }
    if (p > start) {
        vector_push(filter, strdup(start));
    }
    return filter;
}

void TempInit(WINDOW *win) {
    zones = vector_create(ThermalZone, 4);
    memset(zones, 0, 4 * sizeof(ThermalZone));
    VECTOR(char *) filter = TempGetFilter(temp_filter);
    TempDiscover(filter);
    vector_free(filter);
    TempDrawBorder(win);
    WidgetFixedSize(&temp_widget, true);
}

void TempQuit() {
    vector_for_each(zones, zone) {
        free(zone->temp_path);
        free(zone->type);
    }
    vector_free(zones);
}

void TempUpdate() {
    uint64_t total = 0;
    vector_for_each(zones, zone) {
        const int sample = atoi(ReadSmallFile(zone->temp_path, true));
        const int whole = sample / 1000;
        const int decimal = sample % 1000 / 100;
        total += sample / 100;
        snprintf(zone->temp, sizeof(zone->temp), "%5d.%d°C", whole, decimal);
    }
    if (temp_show_average) {
        const size_t count = vector_size(zones);
        const uint64_t average = (total + count / 2) / count;
        const int whole = average / 10;
        const int decimal = average % 10;
        snprintf(average_temp, sizeof(average_temp), "%5d.%d°C", whole, decimal);
    }
}

void TempDraw(WINDOW *win) {
    int width = getmaxx(win) - 2;
    int y = 0;
    vector_for_each(zones, zone) {
        ++y;
        // +1 is to offset the extra byte of the degree symbol
        mvwaddstr(win, y, 1 + width - strlen(zone->temp) + 1, zone->temp);
        mvwaddstr(win, y, 1, zone->type);
    }
    if (temp_show_average) {
        ++y;
        mvwaddstr(win, y, 1 + width - strlen(average_temp) + 1, average_temp);
        mvwaddstr(win, y, 1, "[Average]");
    }
}

void TempResize(WINDOW *win) {
    wclear(win);
    TempDrawBorder(win);
}

void TempMinSize(int *width_return, int *height_return) {
    const int title_min_width = 12 + 2 + 2; // "< Temperature >"
    const int temp_width = 6; // "xx.x°C"
    const int average_min_width = 9 + 2 + temp_width;
    int min_width = Max(title_min_width, average_min_width);
    vector_for_each(zones, zone) {
        min_width = Max(min_width, (int)strlen(zone->type) + 2 + temp_width);
    }
    *width_return = min_width;
    *height_return = vector_size(zones) + temp_show_average;
}

void TempDrawBorder(WINDOW *win) {
    DrawWindow(win, "Temperatures");
}

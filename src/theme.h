#pragma once
#include "ini.h"
#include <stdbool.h>

typedef struct {
    short foreground;
    short background;
} ColorDef;

enum { THEME_CPU_COLORS = 8 };

typedef struct {
    short border;
    short title;
    short cpu_avg;
    short cpu_graphs[THEME_CPU_COLORS];
    short mem_main;
    short mem_swap;
    short net_receive;
    short net_transmit;
    short proc_header;
    short proc_processes;
    short proc_cursor;
    short proc_highlight;
    short proc_high_percent;
    short proc_branches;
    short proc_path;
    short proc_command;
    short proc_opt;
    short proc_arg;
    short disk_free;
    short disk_used;
    short disk_error;
    short battery_fill;
} Theme;

typedef struct ThemeDef ThemeDef;

extern Theme *theme;

/** Define a color pair. */
short DefColor(ColorDef def);

/** Set a color in a theme. */
bool ThemeSet(Theme *self, const char *key, short value);

/** Set values in the given theme from the already parsed config.  BASE may be
    NULL, in which case the default theme is used. */
void ThemeFromConfig(Theme *self, const ThemeDef *base);

/** Returns NULL if the name is a valid named theme definition, and a list of
    valid names otherwise. */
char *VerifyThemeName(const char *name);

/** Get a builtin named theme definition. */
const ThemeDef *NamedThemeDef(const char *name);

/** Create a theme from only a named theme definition. */
Theme *CreateNamedTheme(const char *name);

/** Number of color pairs that can still be created. */
int AvailableColors(void);

/** Reset the pair index used by `DefColor`. */
void ResetColors(void);

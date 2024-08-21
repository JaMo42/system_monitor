#include "theme.h"
#include "config.h"
#include "stdafx.h"

struct ThemeDef {
    ColorDef border;
    ColorDef title;
    ColorDef cpu_avg;
    ColorDef cpu_graphs[THEME_CPU_COLORS];
    ColorDef mem_main;
    ColorDef mem_swap;
    ColorDef net_receive;
    ColorDef net_transmit;
    ColorDef proc_header;
    ColorDef proc_processes;
    ColorDef proc_cursor;
    ColorDef proc_highlight;
    ColorDef proc_high_percent;
    ColorDef proc_branches;
    ColorDef proc_path;
    ColorDef proc_command;
    ColorDef proc_opt;
    ColorDef proc_arg;
    ColorDef disk_free;
    ColorDef disk_used;
    ColorDef disk_error;
    ColorDef battery_fill;

    const char *name;
};

static const char *const FIELD_NAMES[] = {
    "border", "title", "cpu-avg", "cpu-graph-0",
    "cpu-graph-1", "cpu-graph-2", "cpu-graph-3", "cpu-graph-4",
    "cpu-graph-5", "cpu-graph-6", "cpu-graph-7", "mem-main",
    "mem-swap", "net-receive", "net-transmit", "proc-header",
    "proc-processes", "proc-cursor", "proc-highlight", "proc-high-percent",
    "proc-branches", "proc-path", "proc-command", "proc-opt",
    "proc-arg", "disk-free", "disk-used", "disk-error",
    "battery-fill",
};

_Static_assert(sizeof(FIELD_NAMES) / sizeof(*FIELD_NAMES) ==
               sizeof(Theme) / sizeof(short),
               "FIELD_NAMES and Theme must have the same number of fields");

enum { FIELD_COUNT = sizeof(FIELD_NAMES) / sizeof(*FIELD_NAMES) };

#define C(x) ((ColorDef){x, -1})
#define B(x, y) ((ColorDef){x, y})

static const ThemeDef DEFAULT_THEME = {
    .border = C(213),
    .title = C(250),
    .cpu_avg = C(76),
    .cpu_graphs = {C(4), C(3), C(2), C(1), C(5), C(6), C(7), C(8)},
    .mem_main = C(220),
    .mem_swap = C(208),
    .net_receive = C(4),
    .net_transmit = C(1),
    .proc_header = C(15),
    .proc_processes = C(250),
    .proc_cursor = B(0, 76),
    .proc_highlight = B(0, 81),
    .proc_high_percent = C(1),
    .proc_branches = C(242),
    .proc_path = C(244),
    .proc_command = C(-1),
    .proc_opt = C(65),
    .proc_arg = C(60),
    .disk_free = C(247),
    .disk_used = C(76),
    .disk_error = C(1),
    .battery_fill = B(15, 27),

    .name = "default",
};

static const ThemeDef DEFAULT_LIGHT_THEME = {
    .border = C(93),
    .title = C(241),
    .cpu_avg = C(76),
    .cpu_graphs = {C(4), C(3), C(2), C(1), C(5), C(6), C(8), C(0)},
    .mem_main = C(178),
    .mem_swap = C(166),
    .net_receive = C(4),
    .net_transmit = C(1),
    .proc_header = C(236),
    .proc_processes = C(239),
    .proc_cursor = B(0, 76),
    .proc_highlight = B(0, 81),
    .proc_high_percent = C(1),
    .proc_branches = C(242),
    .proc_path = C(244),
    .proc_command = C(-1),
    .proc_opt = C(28),
    .proc_arg = C(25),
    .disk_free = C(239),
    .disk_used = C(34),
    .disk_error = C(1),
    .battery_fill = B(15, 27),

    .name = "default-light",
};

Theme *theme = NULL;

enum { FIRST_COLOR = 1 };

// 0 is already used for the default color
static short G_next_color = FIRST_COLOR;

static short
ParseColor(const char *rep) {
    // TODO: should be faster to not store them in order but storing them with
    // their number in a sorted array and doing a binary search
    static const char *const NAMES[] = {
        "Black",
        "Maroon",
        "Green",
        "Olive",
        "Navy",
        "Purple",
        "Teal",
        "Silver",
        "Grey",
        "Red",
        "Lime",
        "Yellow",
        "Blue",
        "Fuchsia",
        "Aqua",
        "White",
        "Grey0",
        "NavyBlue",
        "DarkBlue",
        "Blue3",
        "Blue3",
        "Blue1",
        "DarkGreen",
        "DeepSkyBlue4",
        "DeepSkyBlue4",
        "DeepSkyBlue4",
        "DodgerBlue3",
        "DodgerBlue2",
        "Green4",
        "SpringGreen4",
        "Turquoise4",
        "DeepSkyBlue3",
        "DeepSkyBlue3",
        "DodgerBlue1",
        "Green3",
        "SpringGreen3",
        "DarkCyan",
        "LightSeaGreen",
        "DeepSkyBlue2",
        "DeepSkyBlue1",
        "Green3",
        "SpringGreen3",
        "SpringGreen2",
        "Cyan3",
        "DarkTurquoise",
        "Turquoise2",
        "Green1",
        "SpringGreen2",
        "SpringGreen1",
        "MediumSpringGreen",
        "Cyan2",
        "Cyan1",
        "DarkRed",
        "DeepPink4",
        "Purple4",
        "Purple4",
        "Purple3",
        "BlueViolet",
        "Orange4",
        "Grey37",
        "MediumPurple4",
        "SlateBlue3",
        "SlateBlue3",
        "RoyalBlue1",
        "Chartreuse4",
        "DarkSeaGreen4",
        "PaleTurquoise4",
        "SteelBlue",
        "SteelBlue3",
        "CornflowerBlue",
        "Chartreuse3",
        "DarkSeaGreen4",
        "CadetBlue",
        "CadetBlue",
        "SkyBlue3",
        "SteelBlue1",
        "Chartreuse3",
        "PaleGreen3",
        "SeaGreen3",
        "Aquamarine3",
        "MediumTurquoise",
        "SteelBlue1",
        "Chartreuse2",
        "SeaGreen2",
        "SeaGreen1",
        "SeaGreen1",
        "Aquamarine1",
        "DarkSlateGray2",
        "DarkRed",
        "DeepPink4",
        "DarkMagenta",
        "DarkMagenta",
        "DarkViolet",
        "Purple",
        "Orange4",
        "LightPink4",
        "Plum4",
        "MediumPurple3",
        "MediumPurple3",
        "SlateBlue1",
        "Yellow4",
        "Wheat4",
        "Grey53",
        "LightSlateGrey",
        "MediumPurple",
        "LightSlateBlue",
        "Yellow4",
        "DarkOliveGreen3",
        "DarkSeaGreen",
        "LightSkyBlue3",
        "LightSkyBlue3",
        "SkyBlue2",
        "Chartreuse2",
        "DarkOliveGreen3",
        "PaleGreen3",
        "DarkSeaGreen3",
        "DarkSlateGray3",
        "SkyBlue1",
        "Chartreuse1",
        "LightGreen",
        "LightGreen",
        "PaleGreen1",
        "Aquamarine1",
        "DarkSlateGray1",
        "Red3",
        "DeepPink4",
        "MediumVioletRed",
        "Magenta3",
        "DarkViolet",
        "Purple",
        "DarkOrange3",
        "IndianRed",
        "HotPink3",
        "MediumOrchid3",
        "MediumOrchid",
        "MediumPurple2",
        "DarkGoldenrod",
        "LightSalmon3",
        "RosyBrown",
        "Grey63",
        "MediumPurple2",
        "MediumPurple1",
        "Gold3",
        "DarkKhaki",
        "NavajoWhite3",
        "Grey69",
        "LightSteelBlue3",
        "LightSteelBlue",
        "Yellow3",
        "DarkOliveGreen3",
        "DarkSeaGreen3",
        "DarkSeaGreen2",
        "LightCyan3",
        "LightSkyBlue1",
        "GreenYellow",
        "DarkOliveGreen2",
        "PaleGreen1",
        "DarkSeaGreen2",
        "DarkSeaGreen1",
        "PaleTurquoise1",
        "Red3",
        "DeepPink3",
        "DeepPink3",
        "Magenta3",
        "Magenta3",
        "Magenta2",
        "DarkOrange3",
        "IndianRed",
        "HotPink3",
        "HotPink2",
        "Orchid",
        "MediumOrchid1",
        "Orange3",
        "LightSalmon3",
        "LightPink3",
        "Pink3",
        "Plum3",
        "Violet",
        "Gold3",
        "LightGoldenrod3",
        "Tan",
        "MistyRose3",
        "Thistle3",
        "Plum2",
        "Yellow3",
        "Khaki3",
        "LightGoldenrod2",
        "LightYellow3",
        "Grey84",
        "LightSteelBlue1",
        "Yellow2",
        "DarkOliveGreen1",
        "DarkOliveGreen1",
        "DarkSeaGreen1",
        "Honeydew2",
        "LightCyan1",
        "Red1",
        "DeepPink2",
        "DeepPink1",
        "DeepPink1",
        "Magenta2",
        "Magenta1",
        "OrangeRed1",
        "IndianRed1",
        "IndianRed1",
        "HotPink",
        "HotPink",
        "MediumOrchid1",
        "DarkOrange",
        "Salmon1",
        "LightCoral",
        "PaleVioletRed1",
        "Orchid2",
        "Orchid1",
        "Orange1",
        "SandyBrown",
        "LightSalmon1",
        "LightPink1",
        "Pink1",
        "Plum1",
        "Gold1",
        "LightGoldenrod2",
        "LightGoldenrod2",
        "NavajoWhite1",
        "MistyRose1",
        "Thistle1",
        "Yellow1",
        "LightGoldenrod1",
        "Khaki1",
        "Wheat1",
        "Cornsilk1",
        "Grey100",
        "Grey3",
        "Grey7",
        "Grey11",
        "Grey15",
        "Grey19",
        "Grey23",
        "Grey27",
        "Grey30",
        "Grey35",
        "Grey39",
        "Grey42",
        "Grey46",
        "Grey50",
        "Grey54",
        "Grey58",
        "Grey62",
        "Grey66",
        "Grey70",
        "Grey74",
        "Grey78",
        "Grey82",
        "Grey85",
        "Grey89",
        "Grey93",
    };
    if ((*rep >= '0' && *rep <= '9') || *rep == '-') {
        return atoi(rep);
    }
    for (size_t i = 0; i < sizeof(NAMES) / sizeof(*NAMES); ++i) {
        if (strcasecmp(rep, NAMES[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static ColorDef
ParseColorDef(const char *rep) {
    ColorDef def = { 0, 0 };
    char *comma = strchr(rep, ',');
    const char *second = comma + 1;
    if (comma) {
        // `rep` comes from the config so it's an allocated string we can mutate
        *comma = '\0';
        while (*second == ' ') {
            ++second;
        }
    }
    if (comma) {
        def.foreground = ParseColor(rep);
        def.background = ParseColor(second);
    } else {
        def.foreground = ParseColor(rep);
        def.background = -1;
    }
    return def;
}

short
DefColor(ColorDef def) {
    const short color = G_next_color++;
    init_pair(color, def.foreground, def.background);
    return color;
}

bool
ThemeSet(Theme *self, const char *key, short value) {
    short *fields = (short *)self;
    for (size_t i = 0; i < FIELD_COUNT; ++i) {
        if (strcmp(key, FIELD_NAMES[i]) == 0) {
            fields[i] = value;
            return true;
        }
    }
    return false;
}

void
ThemeFromConfig(Theme *self, const ThemeDef *base_) {
    // Note: If we have N fields and M items in the config this is O(n) instead
    // of the O(n*m) we would get if we iterated the table and tried setting
    // each item individually, however we ignore any extra items in the table
    // instead of giving warnings about unrecognized fields.
    short *fields = (short *)self;
    ColorDef *base = (ColorDef *)(base_ ? base_ : &DEFAULT_THEME);
    ColorDef def;
    for (size_t i = 0; i < FIELD_COUNT; ++i) {
        Config_Read_Value *value = ConfigGet("theme", FIELD_NAMES[i]);
        if (value) {
            def = ParseColorDef(value->as_string());
        } else {
            def = base[i];
        }
        // We could keep track of existing pairs to avoid duplicates, but
        // as long as we have less fields than there are color slots this
        // does not matter.
        short color = DefColor(def);
        fields[i] = color;
    }
}

const ThemeDef *
NamedThemeDef(const char *name) {
    static const ThemeDef *const THEMES[] = {
        &DEFAULT_THEME,
        &DEFAULT_LIGHT_THEME,
    };
    enum { THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]) };
    for (size_t i = 0; i < THEME_COUNT; ++i) {
        if (strcmp(name, THEMES[i]->name) == 0) {
            return THEMES[i];
        }
    }
    if (strcmp(name, "dark") == 0) {
        return &DEFAULT_THEME;
    } else if (strcmp(name, "light") == 0) {
        return &DEFAULT_LIGHT_THEME;
    }
    endwin();
    fprintf(stderr, "\x1b[1;31merror:\x1b[m unknown theme: %s\n", name);
    fprintf(stderr, "\x1b[1;36mhelp:\x1b[m available themes: ");
    const char *comma = "";
    for (size_t i = 0; i < THEME_COUNT; ++i) {
        fprintf(stderr, "%s%s", comma, THEMES[i]->name);
        comma = ", ";
    }
    fprintf(stderr, "\n");
    exit(1);
}

Theme *
CreateNamedTheme(const char *name) {
    Theme *theme = malloc(sizeof(Theme));
    short *fields = (short *)theme;
    const ThemeDef *def = NamedThemeDef(name);
    ColorDef *defs = (ColorDef *)def;
    for (size_t i = 0; i < FIELD_COUNT; ++i) {
        short color = DefColor(defs[i]);
        fields[i] = color;
    }
    return theme;
}

int
AvailableColors(void) {
    return COLORS - (G_next_color - 1);
}

void
ResetColors(void) {
    G_next_color = FIRST_COLOR;
}

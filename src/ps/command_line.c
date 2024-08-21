#include "command_line.h"
#include "../util.h"

/** Gets the start and end (1 past the last) indices of the comand name. */
static void commandline_get_command_range(char *cmdline, int *command_start, int *command_end) {
    bool ignore = false;
    char c;
    for (int i = 0; ; ++i) {
        c = cmdline[i];
        if (c == '\0') {
            *command_end = i;
            return;
        } else if (c == ':') {
            ignore = true;
        } else if (c == '/' && !ignore) {
            *command_start = i + 1;
        }
    }
}

void commandline_from_cmdline(Command_Line *self, File_Content content) {
    self->data = malloc((content.size + 1) * sizeof(chtype));
    chtype *writeptr = self->data;
    char c;
    int command_start = 0, command_end = 0;
    commandline_get_command_range(content.data, &command_start, &command_end);
    self->command = self->data + command_start;
    int i;
    for (i = 0; i != command_start; ++i) {
        *writeptr++ = content.data[i] | COLOR_PAIR(theme->proc_path);
    }
    for (i = command_start; i != command_end; ++i) {
        *writeptr++ = content.data[i] | COLOR_PAIR(theme->proc_command);
    }
    bool in_opt = content.data[i] == '-';
    attr_t color = COLOR_PAIR(in_opt ? theme->proc_opt : theme->proc_arg);
    for (i = command_end; i < content.size; ++i) {
        c = content.data[i];
        if (c == '\0') {
            in_opt = content.data[i+1] == '-';
            color = COLOR_PAIR(in_opt ? theme->proc_opt : theme->proc_arg);
            *writeptr++ = ' ';
        } else {
            if (in_opt && c == '=') {
                *writeptr++ = c | color;
                color = COLOR_PAIR(theme->proc_arg);
                in_opt = false;
                continue;
            } else if (in_opt && !(isalnum(c) || c == '-')) {
                color = COLOR_PAIR(theme->proc_arg);
                in_opt = false;
            }
            *writeptr++ = c | color;
        }
    }
    *writeptr++ = 0;
    self->str = malloc(content.size + 1);
    for (i = 0; i < content.size; ++i) {
        self->str[i] = (c = content.data[i]) == 0 ? ' ' : c;
    }
    self->str[content.size] = 0;
}

void commandline_from_comm(Command_Line *self, File_Content content) {
    self->data = malloc((content.size + 3) * sizeof(chtype));
    self->command = self->data;
    chtype *writeptr = self->data;
    *writeptr++ = '[' | COLOR_PAIR(theme->proc_path);
    for (int i = 0; i < content.size; ++i) {
        *writeptr++ = content.data[i] | COLOR_PAIR(theme->proc_command);
    }
    self->data[content.size] = ']' | COLOR_PAIR(theme->proc_path);
    self->data[content.size + 1] = 0;
    self->str = malloc(content.size + 1);
    memcpy(self->str, content.data, content.size);
    self->str[content.size] = 0;
}

void commandline_print(
    const Command_Line *self,
    WINDOW *win,
    bool command_only,
    int width,
    bool mask_color
) {
    if (mask_color) {
        chtype *c = command_only ? self->command : self->data;
        for (int i = 0; *c && i < width; ++i) {
            waddch(win, *c++ & 0xff);
        }
    } else {
        waddchnstr(win, command_only ? self->command : self->data, width);
    }
}

bool commandline_contains(const Command_Line *self, const char *needle) {
    return strstr(self->str, needle);
}

void commandline_free(Command_Line *self) {
    free(self->str);
    free(self->data);
}

#pragma once
#include "../stdafx.h"
#include "util.h"

#define COMMANDLINE_ZEROED ((Command_Line) { NULL, NULL, NULL })

typedef struct {
    /** string for searching */
    char *str;
    /** cells for display */
    chtype *data;
    /** points into `data`, skipping the path of the program */
    chtype *command;
} Command_Line;

void commandline_from_file(Command_Line *self, File_Content content);

void commandline_from_comm(Command_Line *self, File_Content comm);

void commandline_print(
    const Command_Line *self,
    WINDOW *win,
    bool command_only,
    int width,
    bool mask_color
);

/** Checks if the string representation of the command line contains the given
    needle string. */
bool commandline_contains(const Command_Line *self, const char *needle);

void commandline_free(Command_Line *self);

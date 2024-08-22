#pragma once
#include "../stdafx.h"
#include "util.h"

#define COMMANDLINE_ZEROED ((Command_Line){NULL, NULL, NULL})

typedef struct {
    /** string for searching */
    char *str;
    /** cells for display */
    chtype *data;
    /** points into `data`, skipping the path of the program */
    chtype *command;
} Command_Line;

/** Initializes the command line with the data from the `cmdline` file. */
void commandline_from_cmdline(Command_Line *self, File_Content content);

/** Initializes the command line with the data from the `comm` file. */
void commandline_from_comm(Command_Line *self, File_Content comm);

/** Prints the command line to the given window.  If `command_only` is `true`
    the path of the command is skipped.  If `mask_color` is true only the
    characters are printed, using the current attributes of the window.  At most
    `width` characters are written. */
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

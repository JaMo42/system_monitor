#pragma once
#include "../stdafx.h"

typedef struct {
    char *data;
    ssize_t size;
} File_Content;

/** Converts a string from one of the proc files to an unsigned integer.  As
    this is for a specific and predictable format there are no checks for
    invalid data.  This also skips the character immediately after the number.
 */
unsigned long str2u(char **endptr);

/** Like `str2u` but for signed integers. */
long str2i(char **endptr);

/** Skips consecutive space (0x20) characters. */
void skipspace(char **endptr);

/** Skips `n` fields of data where the data is any sequence of non-space
    characters separated by a single space. */
void skipfields(char **endptr, int n);

/** Reads up to `n` bytes of a file in a directory. */
File_Content read_file(int dir_fd, const char *filename, int n);

/** Reads up to 511 bytes of a file in a directory. */
File_Content read_entire_file(int dir_fd, const char *filename);

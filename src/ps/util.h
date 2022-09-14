#pragma once
#include "../stdafx.h"

typedef struct {
  char *data;
  ssize_t size;
} File_Content;

unsigned long str2u (char **endptr);

long str2i (char **endptr);

void skipspace (char **endptr);

void skipfields (char **endptr, int n);

File_Content read_file (int dir_fd, const char *filename, int n);

File_Content read_entire_file (int dir_fd, const char *filename);

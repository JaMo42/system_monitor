#include "util.h"

static char file_content_buf[512];

unsigned long str2u (char **endptr)
{
  char c;
  char *s = *endptr;
  unsigned long n = *s - '0';
  // Break on any character less than '0',
  // we need to break on '\0', ' ', '.', and '\n'
  while ((c = *++s) > '/')
    n = n*10 + (c - '0');
  // Skip space after
  *endptr = s + 1;
  return n;
}

long str2i (char **endptr)
{
  if (**endptr == '-') {
    ++(*endptr);
    return -(long)str2u (endptr);
  }
  return str2u (endptr);
}

void skipspace (char **endptr)
{
  while (**endptr == ' ') {
    ++(*endptr);
  }
}

void skipfields (char **endptr, int n)
{
  while (**endptr && n) {
    n -= *(*endptr)++ == ' ';
  }
}

File_Content read_file (int dir_fd, const char *filename, int n)
{
  int fd;
  ssize_t s = -1;
  if ((fd = openat (dir_fd, filename, O_RDONLY)) >= 0) {
    s = read (fd, file_content_buf, n);
    close (fd);
  }
  file_content_buf[s > 0 ? s : 0] = '\n';
  return (File_Content) {
    .data = file_content_buf,
    .size = s
  };
}

File_Content read_entire_file (int dir_fd, const char *filename)
{
  return read_file (dir_fd, filename, sizeof (file_content_buf) - 1);
}

#include "read_stat.h"
#include "globals.h"

bool readstat (int dir_fd, Proc_Stat *stat_out)
{
  File_Content f = read_entire_file (dir_fd, "stat");
  if (f.size < 0) {
    return false;
  }
  char *split = strrchr (f.data, ')');
  split += 4;
  char **p = &split;
  stat_out->parent = str2u (p);
  skipfields (p, 9);
  stat_out->utime = str2u (p);
  stat_out->stime = str2u (p);
  skipfields (p, 6);
  stat_out->start_time = str2u (p);
  skipfields (p, 1);
  stat_out->resident_memory = str2i (p) << page_shift_amount;
  return true;
}

bool readstat_update (int dir_fd, Proc_Stat *stat_out)
{
  File_Content f = read_entire_file (dir_fd, "stat");
  if (f.size < 0) {
    return false;
  }
  char *split = strrchr (f.data, ')');
  split += 4;
  char **p = &split;
  skipfields (p, 10);
  stat_out->utime = str2u (p);
  stat_out->stime = str2u (p);
  skipfields (p, 8);
  stat_out->resident_memory = str2i (p) << page_shift_amount;
  return true;
}

unsigned long get_total_cpu ()
{
  int dir_fd = open ("/proc", O_RDONLY|O_DIRECTORY);
  File_Content f = read_entire_file (dir_fd, "stat");
  close (dir_fd);
  if (f.size < 0) {
    return 0;
  }
  char **p = &f.data;
  skipfields (p, 1);
  skipspace (p);
  return str2u (p)
       + str2u (p)
       + str2u (p)
       + str2u (p)
       + str2u (p)
       + str2u (p)
       + str2u (p)
       + str2u (p)
       + str2u (p)
       + str2u (p);
}

unsigned long get_total_memory ()
{
  int dir_fd = open ("/proc", O_RDONLY|O_DIRECTORY);
  File_Content f = read_file (dir_fd, "meminfo", 40);
  close (dir_fd);
  if (f.size < 0) {
    return 0;
  }
  char *p = f.data + 9;  // Skip 'MemTotal:'
  skipspace (&p);
  // Convert to bytes
  return str2u (&p);
}

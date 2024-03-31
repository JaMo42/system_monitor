#ifndef STDAFX_H
#define STDAFX_H

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>
#include <errno.h>
#include <float.h>
#include <assert.h>

#include <strings.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>

#include <ncurses.h>

#include "c-vector/vector.h"
#include "ini/ini.h"

#include "list.h"

#define PI 3.1415926

#define KB 1024.0f
#define MB (KB*KB)
#define GB (MB*KB)
#define TB (GB*KB)

#define Max(a, b) ({         \
  typeof (a) my_a = a;       \
  typeof (b) my_b = b;       \
  my_a > my_b ? my_a : my_b; \
})

#define Min(a, b) ({         \
  typeof (a) my_a = a;       \
  typeof (b) my_b = b;       \
  my_a < my_b ? my_a : my_b; \
})

#define Clamp(x, low, high) Max ((low), Min ((x), (high)))

#define InRange(x, begin, end) (((x) >= (begin)) && ((x) < (end)))

#define likely(x) __builtin_expect((x), true)
#define unlikely(x) __builtin_expect((x), false)

#define countof(arr) (sizeof (arr) / sizeof (*(arr)))

#endif /* !STDAFX_H */

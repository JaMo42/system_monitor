#ifndef STDAFX_H
#define STDAFX_H

#define _GNU_SOURCE

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "c-vector/vector.h"
#include "ini/ini.h"
#include "list.h"
#include "theme.h"

#define PI 3.1415926

#define KB 1024.0f
#define MB (KB * KB)
#define GB (MB * KB)
#define TB (GB * KB)

#define Max(a, b)              \
  ({                           \
    typeof(a) my_a = a;        \
    typeof(b) my_b = b;        \
    my_a > my_b ? my_a : my_b; \
  })

#define Min(a, b)              \
  ({                           \
    typeof(a) my_a = a;        \
    typeof(b) my_b = b;        \
    my_a < my_b ? my_a : my_b; \
  })

#define Clamp(x, low, high) Max((low), Min((x), (high)))

#define InRange(x, begin, end) (((x) >= (begin)) && ((x) < (end)))

#define likely(x) __builtin_expect((x), true)
#define unlikely(x) __builtin_expect((x), false)

#define countof(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif /* !STDAFX_H */

#ifndef STDAFX_H
#define STDAFX_H

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

#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <ifaddrs.h>
#include <pthread.h>

#include <ncurses.h>

#include "list.h"

#include "ic.h"

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

#ifdef __GNUC__
#define likely(x) __builtin_expect((x), true)
#define unlikely(x) __builtin_expect((x), false)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define countof(arr) (sizeof (arr) / sizeof (*(arr)))

#endif /* !STDAFX_H */

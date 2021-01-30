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

#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <ifaddrs.h>
#include <pthread.h>

#include <ncurses.h>

#include "list.h"

#define sm_clamp(x, low, high) (((x) < (low)) ? (low) : (((x) > (high)) ? (high) : (x)))

#ifdef __GNUC__
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#endif /* !STDAFX_H */

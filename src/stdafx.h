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

#include <ncurses.h>
#include <fmt/fmt.h>

#include "list.h"

#define sm_clamp(x, low, high) (((x) < (low)) ? (low) : (((x) > (high)) ? (high) : (x)))

#endif /* !STDAFX_H */

/* Configuration options for JOE */

#ifndef _Iconfig
#define _Iconfig 1

#if USE_PTS_DARWIN
#  include "autoconf-darwin.h"
#  define NEED_UTIL_H_FOR_LOGIN_TTY 1
#  define NEED_UTIL_H_FOR_OPENPTY 1
#  define HAVE_ALL_MATH 1
#endif

#if USE_PTS_LINUX
#  include "autoconf-linux.h"
#endif

#define USEPTMX 1
#define MOUSE_XTERM 1

#endif

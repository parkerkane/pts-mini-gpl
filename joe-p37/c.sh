#! /bin/bash --
#
# c.sh -- shell script to compile joe-p37 on Linux and the Mac OS X
# by pts@fazekas.hu at Sat Nov  8 15:48:26 CET 2008
#
set -ex

UNAME=$(uname)

WFLAGS='-Wall -W -Wno-unused-parameter -Wno-format'
SRCS='b.c blocks.c bw.c cmd.c hash.c help.c kbd.c macro.c main.c menu.c path.c poshist.c pw.c queue.c qw.c rc.c regex.c scrn.c tab.c termcap.c tty.c tw.c ublock.c uedit.c uerror.c ufile.c uformat.c uisrch.c umath.c undo.c usearch.c ushell.c utag.c va.c vfile.c vs.c w.c utils.c syntax.c utf8.c selinux.c i18n.c charmap.c mouse.c lattr.c gettext.c builtin.c builtins.c'

if test "$UNAME" = Darwin; then
  # Same as in joe-3.7 in MacPorts.
  DIRFLAGS='-DJOERC="/opt/local/etc/joe/" -DJOEDATA="/opt/local/share/joe/" '
else
  # DIRFLAGS='-DJOERC="/usr/local/etc/joe-pts/" -DJOEDATA="/usr/local/share/joe-pts/" '
  # Ubuntu Lucid defaults:
  DIRFLAGS='-DJOERC="/etc/joe/" -DJOEDATA="/usr/share/joe/" '
fi

if test "$CC"; then
  :
elif test "$UNAME" = Darwin; then
  # Mac OS X 10.5 support added to c.sh at Sun Oct 16 14:36:02 CEST 2011
  # Darwin lambda-wifi.lan 9.8.0 Darwin Kernel Version 9.8.0: Wed Jul 15 16:55:01 PDT 2009; root:xnu-1228.15.4~1/RELEASE_I386 i386
  # CC='gcc-mp-4.4 -static-libgcc' ./configure --disable-curses --disable-termcap
  # gcc-mp-4.4 from MacPorts
  CC='gcc-mp-4.4 -static-libgcc' 
else
  if type -p i386-uclibc-gcc >/dev/null; then
    CC='i386-uclibc-gcc -m32 -static'
  else
    CC=gcc
  fi
fi

if test "$UNAME" = Darwin; then
  SYSFLAGS='-DUSE_PTS_DARWIN'
elif test "$UNAME" = Linux; then
  SYSFLAGS='-DUSE_PTS_LINUX'
else
  SYSFLAGS=''
fi

if test "$1"; then  # --debug
  CFLAGS='-g'
  STRIP=:
else
  CFLAGS='-O2'
  STRIP=strip
fi

$CC -o joe $SYSFLAGS $WFLAGS $CFLAGS $DIRFLAGS $SRCS -lm
$STRIP joe
if test "$UNAME" != Darwin; then
  perl -x elfosfix.pl joe
fi
ls -l joe
: joe-p37 compilation OK.

#! /bin/bash --
# by pts@fazekas.hu at Sat Nov  8 15:48:26 CET 2008
set -ex
INCLUDEFLAGS='-DHAVE_CONFIG_H -I. '
# DIRFLAGS='-DJOERC="/usr/local/etc/joe-pts/" -DJOEDATA="/usr/local/share/joe-pts/" '
# Ubuntu Lucid defaults:
DIRFLAGS='-DJOERC="/etc/joe/" -DJOEDATA="/usr/share/joe/" '
WFLAGS='-Wall -W -Wno-unused-parameter -Wno-format'
CC="${CC:-i386-uclibc-gcc -static}"
if test "$1"; then  # --debug
  CFLAGS='-g'
else
  CFLAGS='-s -O2'
fi
SRCS='b.c blocks.c bw.c cmd.c hash.c help.c kbd.c macro.c main.c menu.c path.c poshist.c pw.c queue.c qw.c rc.c regex.c scrn.c tab.c termcap.c tty.c tw.c ublock.c uedit.c uerror.c ufile.c uformat.c uisrch.c umath.c undo.c usearch.c ushell.c utag.c va.c vfile.c vs.c w.c utils.c syntax.c utf8.c selinux.c i18n.c charmap.c mouse.c lattr.c gettext.c builtin.c builtins.c'
#$CC -o joe $INCLUDEFLAGS $CFLAGS $SRCS -lm
$CC -o joe $INCLUDEFLAGS $WFLAGS $CFLAGS $DIRFLAGS $SRCS -lm
perl -x elfosfix.pl joe

#! /bin/bash --
#
# build script for the jbig2 tool
# by pts@fazekas.hu at Fri Aug 26 21:09:16 CEST 2011
#

if true; then  # Make the shell script editable while it's executing.

test "${0%/*}" != "$0" && cd "${0%/*}"  

BUILDDIR=jbig2.build
PBUILDDIR="$PWD/$BUILDDIR"
UNAME=$(./busybox uname 2>/dev/null || uname || true)

export LDFLAGS="-L$PBUILDDIR/build-lib"
if test "$UNAME" = Darwin; then
  export CC="gcc-mp-4.4 -m32 -static-libgcc -I$PBUILDDIR/build-include"
  export CXX="g++-mp-4.4 -m32 -static-libgcc -I$PBUILDDIR/build-include -fno-rtti -fno-exceptions"
  export AR=ar
  export RANLIB=ranlib
  export LD=ld
  export STRIP=strip
else
  # Example: CROSS_COMPILER_PREFIX=.../cross-compiler-i686/bin/i686-gcc
  # !!
  if test ! -e "${CROSS_COMPILER_PREFIX}ar"; then
    echo "error: please set CROSS_COMPILER_PREFIX properly" >&2
    exit 1
  fi
  export CC="${CROSS_COMPILER_PREFIX}gcc -static -fno-stack-protector -I$PBUILDDIR/build-include"
  export CXX="${CROSS_COMPILER_PREFIX}g++ -static -fno-stack-protector -I$PBUILDDIR/build-include -fno-rtti -fno-exceptions"
  export AR="${CROSS_COMPILER_PREFIX}ar"
  export RANLIB="${CROSS_COMPILER_PREFIX}ranlib"
  export LD="${CROSS_COMPILER_PREFIX}ld"  # The ./configure script of libevent2 fails without $LD being set.
  export STRIP="${CROSS_COMPILER_PREFIX}strip -s"
  echo $CC
fi

echo "Running in directory: $PWD"

set -x

function initbuilddir() {
  rm -rf "$BUILDDIR" || return "$?"
  mkdir "$BUILDDIR" || return "$?"
  mkdir "$BUILDDIR/build-lib" || return "$?"
  mkdir "$BUILDDIR/build-include" || return "$?"
}

buildlibz() {
  ( cd "$BUILDDIR" || return "$?"
    rm -rf zlib-1.2.5 || return "$?"
    tar xzvf ../zlib-1.2.5.tar.gz || return "$?"
    cd zlib-1.2.5 || return "$?"
    ./configure --static || return "$?"
    perl -pi~ -e 's@\s-g(?!\S)@@g, s@\s-O\d*(?!\S)@ -O2@g if s@^CFLAGS\s*=@CFLAGS = @' Makefile || return "$?"
    make libz.a || return "$?"
    cp libz.a ../build-lib/libz-jbib.a || return "$?"
    cp zconf.h zlib.h ../build-include/ || return "$?"
  ) || return "$?"
}

buildlibpng() {
  ( cd "$BUILDDIR" || return "$?"
    rm -rf libpng-1.2.46 || return "$?"
    tar xzvf ../libpng-1.2.46.tar.gz || return "$?"
    cd libpng-1.2.46 || return "$?"
    perl -pi~ -e 's@([ "])-lz(?=[ ".])@$1-lz-jbib@g' configure || return "$?"
    ./configure --disable-shared || return "$?"
    perl -pi~ -e 's@\s-g(?!\S)@@g, s@\s-O\d*(?!\S)@ -O2@g if s@^CFLAGS\s*=@CFLAGS = @' Makefile || return "$?"
    make libpng.la || return "$?"
    cp .libs/libpng.a ../build-lib/libpng-jbib.a || return "$?"
    cp png.h pngconf.h ../build-include/ || return "$?"
  ) || return "$?"
}

buildliblept() {
  # TODO(pts): Build only parts of liblept?
  ( cd "$BUILDDIR" || return "$?"
    rm -rf leptonica-1.68 || return "$?"
    tar xzvf ../leptonica-1.68.tar.gz || return "$?"
    cd leptonica-1.68 || return "$?"
    #perl -pi~ -e 's@([ "]-l(?:z|png))(?=[ ".])@$1-jbib@g' configure || return "$?"
    perl -pi~ -e 's@([ "])-lz(?=[ ".])@$1-lz-jbib@g' configure || return "$?"
    perl -pi~ -e 's@([ "])-lpng(?=[ ".])@$1-lpng-jbib -lz-jbib -lm@g' configure || return "$?"
    ./configure --disable-shared --with-zlib --with-libpng --without-jpeg --without-giflib --without-libtiff --disable-shared || return "$?"
    perl -pi~ -e 's@\s-g(?!\S)@@g, s@\s-O\d*(?!\S)@ -O2@g if s@^CFLAGS\s*=@CFLAGS = @' Makefile src/Makefile || return "$?"
    make -C src liblept.la || return "$?"
    cp src/.libs/liblept.a ../build-lib/liblept-jbib.a || return "$?"
    #!!cp png.h pngconf.h ../build-include/ || return "$?"
  ) || return "$?"
}

buildjbig2() {
  ( cd "$BUILDDIR" || return "$?"
    rm -rf agl-jbig2enc-e8be922 || return "$?"
    tar xzvf ../agl-jbig2enc-0.27-20-ge8be922.tar.gz || return "$?"
    cd agl-jbig2enc-e8be922 || return "$?"
    for F in jbig2.cc jbig2enc.cc jbig2arith.cc jbig2sym.cc; do
      $CXX -O2 -W -Wall -I../leptonica-1.68/src -c "$F" || return "$?"
    done
    $CXX $LDFLAGS -o jbig2 jbig2.o jbig2enc.o jbig2arith.o jbig2sym.o -llept-jbib -lpng-jbib -lz-jbib -lm || return "$?"
    $STRIP jbig2 || return "$?"
    ls -l jbig2 || return "$?"
    cp jbig2 .. || return "$?"
  ) || return "$?"
}

function compress() {
  cp "$BUILDDIR/agl-jbig2enc-e8be922/jbig2" "$BUILDDIR/jbig2" || return "$?"
  $STRIP "$BUILDDIR/jbig2" || return "$?"
  test "$UNAME" = Linux && ./upx.linux --brute "$BUILDDIR/jbig2"  # !! Mac
  test "$UNAME" = Linux && do_elfosfix "$BUILDDIR/jbig2"
  ls -l "$BUILDDIR/jbig2" || return "$?"
}
function copyjbig2() {
  cp "$BUILDDIR/jbig2" jbig2 || return "$?"
}

# Fix ELF binaries to contain GNU/Linux as the operating system. This is
# needed when running the program on FreeBSD in Linux mode.
do_elfosfix() {
  perl -e'
use integer;
use strict;

#** ELF operating system codes from FreeBSDs /usr/share/misc/magic
my %ELF_os_codes=qw{
SYSV 0
HP-UX 1
NetBSD 2
GNU/Linux 3
GNU/Hurd 4
86Open 5
Solaris 6
Monterey 7
IRIX 8
FreeBSD 9
Tru64 10
Novell 11
OpenBSD 12
ARM 97
embedded 255
};
my $from_oscode=$ELF_os_codes{"SYSV"};
my $to_oscode=$ELF_os_codes{"GNU/Linux"};

for my $fn (@ARGV) {
  my $f;
  if (!open $f, "+<", $fn) {
    print STDERR "$0: $fn: $!\n";
    exit 2  # next
  }
  my $head;
  # vvv Imp: continue on next file instead of die()ing
  die if 8!=sysread($f,$head,8);
  if (substr($head,0,4)ne"\177ELF") {
    print STDERR "$0: $fn: not an ELF file\n";
    close($f); next;
  }
  if (vec($head,7,8)==$to_oscode) {
    print STDERR "$0: info: $fn: already fixed\n";
  }
  if ($from_oscode!=$to_oscode && vec($head,7,8)==$from_oscode) {
    vec($head,7,8)=$to_oscode;
    die if 0!=sysseek($f,0,0);
    die if length($head)!=syswrite($f,$head);
  }
  die "file error\n" if !close($f);
}' -- "$@"
}

function do_build() {
  initbuilddir || return "$?"
  buildlibz || return "$?"  
  buildlibpng || return "$?"  
  buildliblept || return "$?"  
  buildjbig2 || return "$?"  
  compress || return "$?"
  copyjbig2 || return "$?"
}

do_build
exit "$?"

fi

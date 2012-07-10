#! /bin/bash --
#
# build-mingw.sh -- jbig2 build script for Win32
# by pts@fazekas.hu at Tue Jul 10 18:28:30 CEST 2012
#
# Got *-dev-mingw32-components-9.2.tar.gz from
# http://nuwen.net/files/mingw/components-9.2.7z

set -ex

# `sudo apt-get install ming32' on Ubuntu Lucid.
type -p i586-mingw32msvc-g++
type -p i586-mingw32msvc-gcc
type -p i586-mingw32msvc-ar
type -p i586-mingw32msvc-ranlib

rm -rf build.win32
mkdir build.win32
cd build.win32
tar xzvf ../leptonica-1.68.tar.gz
patch -p0 <../leptonica-1.68.mingw.diff
tar xzvf ../agl-jbig2enc-0.27-20-ge8be922.tar.gz
patch -p0 <../agl-jbig2enc-0.27-20-ge8be922.mingw.diff
mv agl-jbig2enc-e8be922 jbig2enc
tar xzvf ../libpng-1.5.11-dev-ming32-components-9.2.tar.gz
tar xzvf ../zlib-1.2.7-dev-mingw32-components-9.2.tar.gz

rm -rf leptonica-1.68/src/*.o
(cd leptonica-1.68/src && i586-mingw32msvc-gcc -mconsole -O2 -c \
                -I../../include \
		adaptmap.c affine.c affinecompose.c \
		arithlow.c arrayaccess.c \
		bardecode.c baseline.c bbuffer.c \
		bilinear.c binarize.c \
		binexpand.c binexpandlow.c \
		binreduce.c binreducelow.c \
		blend.c bmf.c bmpio.c bmpiostub.c \
		boxbasic.c boxfunc1.c boxfunc2.c boxfunc3.c \
		bytearray.c ccbord.c ccthin.c classapp.c \
		colorcontent.c colormap.c colormorph.c \
		colorquant1.c colorquant2.c \
		colorseg.c colorspace.c \
		compare.c conncomp.c convertfiles.c \
		convolve.c convolvelow.c correlscore.c \
		dewarp.c dwacomb.2.c dwacomblow.2.c \
		edge.c enhance.c \
		fhmtauto.c fhmtgen.1.c fhmtgenlow.1.c \
		finditalic.c flipdetect.c fliphmtgen.c \
		fmorphauto.c fmorphgen.1.c fmorphgenlow.1.c \
		fpix1.c fpix2.c \
		gifio.c gifiostub.c \
		gplot.c graphics.c \
		graymorph.c graymorphlow.c \
		grayquant.c grayquantlow.c heap.c \
		jbclass.c jpegio.c jpegiostub.c \
		kernel.c libversions.c list.c maze.c \
		morph.c morphapp.c morphdwa.c morphseq.c \
		numabasic.c numafunc1.c numafunc2.c \
		pageseg.c paintcmap.c \
		parseprotos.c partition.c \
		pdfio.c pdfiostub.c \
		pix1.c pix2.c pix3.c pix4.c pix5.c \
		pixabasic.c pixacc.c \
		pixafunc1.c pixafunc2.c \
		pixalloc.c pixarith.c \
		pixcomp.c pixconv.c pixtiling.c \
		pngio.c pngiostub.c pnmio.c pnmiostub.c \
		projective.c psio1.c psio1stub.c \
		psio2.c psio2stub.c \
		ptabasic.c ptafunc1.c \
                ptra.c queue.c quadtree.c rank.c \
		readbarcode.c readfile.c regutils.c \
		rop.c ropiplow.c roplow.c \
		rotate.c rotateam.c rotateamlow.c \
		rotateorth.c rotateorthlow.c rotateshear.c \
		runlength.c sarray.c \
		scale.c scalelow.c \
		seedfill.c seedfilllow.c \
		sel1.c sel2.c selgen.c \
		shear.c skew.c spixio.c \
		stack.c sudoku.c \
		textops.c tiffio.c tiffiostub.c \
		utils.c viewfiles.c \
		warper.c watershed.c \
		webpio.c webpiostub.c writefile.c \
		zlibmem.c zlibmemstub.c
) || exit "$?"
(cd leptonica-1.68/src && i586-mingw32msvc-ar cr liblept.a *.o) || exit "$?"
i586-mingw32msvc-ranlib leptonica-1.68/src/liblept.a

rm -f jbig2enc/*.o
(cd jbig2enc && i586-mingw32msvc-g++ -mconsole -O2 -I../leptonica-1.68/src \
    -c jbig2.cc jbig2arith.cc jbig2enc.cc jbig2sym.cc) || exit "$?"
(cd jbig2enc && i586-mingw32msvc-g++ -mconsole -s -o jbig2.exe jbig2*.o \
    -L../leptonica-1.68/src -llept -L../lib -lpng -lz)
cd ..
mv jbig2.exe{,.old} || true
cp build.win32/jbig2enc/jbig2.exe ./
ls -l jbig2.exe
: jbig2.exe compiled OK.

#!/bin/sh

CC=gcc
CFLAGS="-Wall -Wshadow -std=gnu99 -I. "
XLDFLAGS="-Wl,-s"
cd $(dirname $0)


# mingw(32bit)
XCFLAGS="-Os -g0 -march=i686 -msse2 -mfpmath=sse -ffast-math -fexcess-precision=fast -fomit-frame-pointer"
LDFLAGS="-shared -Wl,--dll,--add-stdcall-alias -L."

rm -f scenechange.dll temporalsoften2.dll
$CC -o scenechange.dll $CFLAGS $XCFLAGS $LDFLAGS $XLDFLAGS scenechange.c
$CC -o temporalsoften2.dll $CFLAGS $XCFLAGS $LDFLAGS $XLDFLAGS temporalsoften.c

# Linux (64bit)
#CFLAGS="$CFLAGS -fPIC"
#XCFLAGS="-Os -g0 -ffast-math -fexcess-precision=fast -fomit-frame-pointer"
#LDFLAGS="-shared -fPIC -L."
#
#rm -f libscenechange.so libtemporalsoften2.so
#$CC -o libscenechange.so $CFLAGS $XCFLAGS $LDFLAGS $XLDFLAGS scenechange.c
#$CC -o libtemporalsoften2.so $CFLAGS $XCFLAGS $LDFLAGS $XLDFLAGS temporalsoften.c

# Mac
# ?

# MSVC10
# rename *.c to *.cpp
# make project files by yourself

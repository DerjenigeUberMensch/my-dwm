# WM version
VERSION = 6.4

# Customize below to fit your system

# paths
PREFIX = /usr/local/
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# Xinerama, comment if you don't want it
XINERAMALIBS  = -lXinerama
XINERAMAFLAGS = -DXINERAMA

# lXrender (window icons)
XRENDER = -lXrender
IMLIB2LIBS = -lImlib2

# freetype
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2
# OpenBSD (uncomment)
#FREETYPEINC = ${X11INC}/freetype2
#MANPREFIX = ${PREFIX}/man

# includes and libs
INCS = -I${X11INC} -I${FREETYPEINC}
LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS} ${FREETYPELIBS} ${IMLIB2LIBS} ${XRENDER}

# flags
# WARN: WHEN DEBUGGING USING -pg / other gcc debugging settings CRASHES WILL occur when restarting
# -g -> debug

#TODO FIX x86 SUPPORT
X86SUPPORT = -std=c99
STRIPFLAGS = -Wl,--strip-all,-s 
DEBUGFLAGS = -ggdb -g -pg 
WARNINGFLAGS = -pedantic -Wall -Wextra -Wno-deprecated-declarations -Wshadow -Wmaybe-uninitialized 
SIZEFLAGS  = -ffunction-sections -fdata-sections
#Do note that compiling this way leads to MASSIVE binaries size, hence "debug"
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
cFLAGS   = -std=c99 ${WARNINGFLAGS} ${INCS} ${CPPFLAGS} ${X86SUPPORT}
RELEASEFLAGS = ${cFLAGS} ${STRIPFLAGS} -ftree-vectorize

DEBUG 	= ${cFLAGS} ${DEBUGFLAGS} -O0
SIZE  	= ${RELEASEFLAGS} -Os
SIZEONLY= ${RELEASEFLAGS} ${SIZEFLAGS} -Oz
#Release Stable (-O2)
RELEASE = ${RELEASEFLAGS} -O2
#Release Speed (-O3)
RELEASES= ${RELEASEFLAGS} -O3

#Set your options or presets
CFLAGS = ${RELEASES}

# Solaris
#CFLAGS  = -fast ${INCS} -DVERSION=\"${VERSION}\"

LDFLAGS  = ${LIBS}

# compiler and linker
CC = cc

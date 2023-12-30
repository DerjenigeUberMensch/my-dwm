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

#X86 isnt explicitly supported and some code might need to be tweaked
X86SUPPORT = -m32
X64SUPPORT = -march=x86-64 -mtune=generic
SELFFLAGS  = -march=native -mtune=native
STRIPFLAGS = -Wl,--strip-all -s
DEBUGFLAGS = -ggdb -g -pg 
WARNINGFLAGS = -pedantic -Wall -Wextra -Wno-deprecated-declarations -Wshadow -Wmaybe-uninitialized 
SIZEFLAGS  = -ffunction-sections -fdata-sections -finline-functions

CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
cFLAGS   = -std=c99 ${WARNINGFLAGS} ${INCS} ${CPPFLAGS} ${X64SUPPORT}
#-flto saves a couple instructions in certain functions; 
RELEASEFLAGS = ${cFLAGS} ${STRIPFLAGS} -flto

DEBUG 	= ${cFLAGS} ${DEBUGFLAGS} -O0
SIZE  	= ${RELEASEFLAGS} -Os
SIZEONLY= ${RELEASEFLAGS} ${SIZEFLAGS} -Oz

#Release Stable (-O2)
RELEASE = ${RELEASEFLAGS} -ftree-vectorize -O2
#Release Speed (-O3)
RELEASES= ${RELEASEFLAGS} -O3

#Build using cpu specific instruction set for more performance (Optional)
BUILDSELF = ${RELEASEFLAGS} ${SELFFLAGS} -O3

#Set your options or presets (see above) ex: ${PRESETNAME}
CFLAGS = ${RELEASES}

# Solaris
#CFLAGS  = -fast ${INCS} -DVERSION=\"${VERSION}\"

LDFLAGS  = ${LIBS}

# compiler and linker
CC = cc

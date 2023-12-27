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
STRIPFLAGS = -fdata-sections -ffunction-sections -Wl,--strip-all,--gc-sections -s 
WARNINGFLAGS = -pedantic -Wall -Wextra -Wno-deprecated-declarations -Wshadow
#Do note that compiling this way leads to MASSIVE binaries size, hence "debug"
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
#CFLAGS   = -ggdb -g -std=c99 ${WARNINGFLAGS} ${INCS} ${CPPFLAGS} -O2



# SZ 
#CFLAGS  = -std=c99 ${WARNINGFLAGS} ${STRIPFLAGS} ${INCS} ${CPPFLAGS} -Os
# SZ only 
#CFLAGS  = -std=c99 ${WARNINGFLAGS} ${STRIPFLAGS} ${INCS} ${CPPFLAGS} -Oz


# Release
CFLAGS  = -std=c99 -ftree-vectorize ${WARNINGFLAGS} ${STRIPFLAGS} ${INCS} ${CPPFLAGS} -O2
# Release Speed 
#CFLAGS  = -std=c99 ${WARNINGFLAGS} ${STRIPFLAGS} ${INCS} ${CPPFLAGS} -O3

# Solaris
#CFLAGS  = -fast ${INCS} -DVERSION=\"${VERSION}\"
#DEBUG 
#LDFLAGS = ${LIBS}
#Release
LDFLAGS  = ${LIBS}

# compiler and linker
CC = cc

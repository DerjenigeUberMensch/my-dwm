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
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
CFLAGS   = -ggdb -g -std=c99 -pedantic -Wall -Wextra -Wshadow ${INCS} ${CPPFLAGS} -O0


# SZ
#CFLAGS  = -s -std=c99 -pedantic -Wall -Wno-deprecated-declarations -Wshadow ${INCS} ${CPPFLAGS} -Os

# Release
CFLAGS  = -s -std=c99 -pedantic -Wall -Wno-deprecated-declarations -Wshadow -ftree-vectorize ${INCS} ${CPPFLAGS} -O2

# Solaris
#CFLAGS  = -fast ${INCS} -DVERSION=\"${VERSION}\"
#DEBUG 
#LDFLAGS = ${LIBS}
#Release
LDFLAGS  = ${LIBS}

# compiler and linker
CC = cc


# compiler 
CC = cc

# paths
PREFIX = /usr/local/
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib
# Xinerama, comment if you don't want it
XINERAMALIBS  = -lXinerama
XINERAMAFLAGS = -DXINERAMA
LIMLIB2 = -lImlib2

# lXrender (window icons)
XRENDER = -lXrender
# Cursor fonts
XCUR = -lXcursor
# freetype
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2
# OpenBSD (uncomment)
#FREETYPEINC = ${X11INC}/freetype2
#MANPREFIX = ${PREFIX}/man

# includes and libs
INCS = -I${X11INC} -I${FREETYPEINC}
LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS} ${FREETYPELIBS} ${XRENDER} ${XCUR} ${LIMLIB2}

#X86 isnt explicitly supported and some code might need to be tweaked
# Mainly the lbmi2 thing where its only used for resizing an icon so you can just not resize icons and remove that
X86 = -m32
X64 = -march=x86-64 -mtune=generic
XNATIVE = -march=native -mtune=native
STATICLINK = -static
DYNAMICLINK= -ldl
SECTIONCODE= -ffunction-sections -fdata-sections
DEBUGFLAGS = -ggdb -g -pg 

WARNINGFLAGS = -pedantic -Wall -Wno-deprecated-declarations -Wshadow -Wuninitialized
PRELINKERFLAGS = -flto -fipa-pta -fprefetch-loop-arrays 
# can set higher but function overhead is pretty small so meh
INLINELIMIT = 15
LINKERFLAGS = ${DYNAMICLINK} -Wl,--gc-sections,--as-needed,--relax,--strip-all -finline-functions -finline-limit=${INLINELIMIT} -flto 

BINARY = ${X64}
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L ${XINERAMAFLAGS}
CCFLAGS  = -std=c99 ${WARNINGFLAGS} ${INCS} ${CPPFLAGS} ${BINARY} ${PRELINKERFLAGS} ${SECTIONCODE} 
RELEASEFLAGS = ${CCFLAGS} 

DEBUG 	= ${CCFLAGS} ${DEBUGFLAGS} -O0
# uncomment for debugging
LINKERFLAGS = ${DYNAMICLINK} -Wl,--gc-sections

SIZE  	= ${RELEASEFLAGS} -Os
# This rarely saves a substantial amount of instructions
SIZEONLY= ${RELEASEFLAGS} -Oz

# Release Stable (-O2)
RELEASE = ${RELEASEFLAGS} -O2
# Release Speed (-O3)
RELEASES= ${RELEASEFLAGS} -O3 

# Build using cpu specific instruction set for more performance (Optional)
BUILDSELF = ${RELEASEFLAGS} ${XNATIVE} -O3

# Set your options or presets (see above) ex: ${PRESETNAME} (Compiler used is on top)
CFLAGS = ${RELEASES}
# Linker flags
LDFLAGS =  ${LIBS} ${LINKERFLAGS} ${BINARY} 
# Solaris
#CFLAGS  = -fast ${INCS} -DVERSION=\"${VERSION}\"

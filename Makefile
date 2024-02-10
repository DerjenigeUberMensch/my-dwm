# See LICENSE file for copyright and license details.

include config.mk

SRC = drw.c dwm.c util.c toggle.c events.c          pool.c winutil.c 
SRCH= drw.h dwm.h util.h toggle.h events.h config.h pool.h winutil.h keybinds.h
OBJ = ${SRC:.c=.o}
VERSION = XXX
EXE = dwm
all: options default

options:
	@echo dwm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@echo "rebuild  = ${REBUILD}"

.c.o:
	${CC} -c ${CFLAGS} $<

default: ${OBJ}
	${CC} -o ${EXE} ${OBJ} ${LDFLAGS}
	rm -f -- *.o

release:
	rm -rf -f -- ${EXE}-${VERSION}
	cp -R . ${EXE}-${VERSION}
#	tar -cf ${EXE}-${VERSION}.tar ${EXE}-${VERSION}
#	gzip ${EXE}-${VERSION}.tar

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${EXE} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${EXE}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/${EXE}

.PHONY: all options release dist install uninstall 

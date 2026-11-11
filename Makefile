include config.mk

all: modbar

debug: CFLAGS = ${DBG_CFLAGS}
debug: LDFLAGS = ${DBG_LDFLAGS}
debug: modbar

modbar: modbar.c config.h
	${CC} -o $@ modbar.c ${CFLAGS} ${LDFLAGS}

clean:
	rm -f modbar *.o

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f modbar ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/modbar

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/modbar

.PHONY: all clean install uninstall debug

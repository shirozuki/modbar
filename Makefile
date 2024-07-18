include config.mk

all: modbar

modbar: modbar.c config.h
	${CC} -o modbar modbar.c ${CFLAGS} ${LDFLAGS}

clean:
	rm -f *.o modbar

install: modbar
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f modbar ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/modbar

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/modbar

.PHONY: all clean install uninstall
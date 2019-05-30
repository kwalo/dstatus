# See LICENSE file for copyright and license details
# dstatus - display time and volume on dwm status line
.POSIX:

include config.mk

all: dstatus

dstatus.o: dstatus.c
mixer.o: mixer.c

dstatus: dstatus.o mixer.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

.c.o:
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $(LDLIBS) $< -o $@

clean:
	rm -f dstatus dstatus.o mixer.o

dist:
	rm -rf "dstatus-$(VERSION)"
	mkdir -p "dstatus-$(VERSION)"
	cp -R LICENSE Makefile README config.mk config.def.h \
	      dstatus.c \
	      "dstatus-$(VERSION)"
	tar -cf - "dstatus-$(VERSION)" | gzip -c > "dstatus-$(VERSION).tar.gz"
	rm -rf "dstatus-$(VERSION)"

install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp -f dstatus "$(DESTDIR)$(PREFIX)/bin"
	chmod 755 "$(DESTDIR)$(PREFIX)/bin/dstatus"
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man1"
	cp -f dstatus.1 "$(DESTDIR)$(MANPREFIX)/man1"
	chmod 644 "$(DESTDIR)$(MANPREFIX)/man1/dstatus.1"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/dstatus"
	rm -f "$(DESTDIR)$(MANPREFIX)/man1/dstatus.1"

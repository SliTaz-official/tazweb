# Makefile for TazLauncher.
#
PREFIX?=/usr
DOCDIR?=$(PREFIX)/share/doc
DESTDIR?=

PACKAGE=tazweb
VERSION=1.0

all:
	gcc src/main.c -o $(PACKAGE) \
		`pkg-config --cflags --libs gtk+-2.0 webkit-1.0`
	@du -sh $(PACKAGE)

install:
	mkdir -p \
		$(DESTDIR)$(DOCDIR)/$(PACKAGE) \
		$(DESTDIR)$(PREFIX)/bin \
		$(DESTDIR)$(PREFIX)/share/pixmaps \
		$(DESTDIR)$(PREFIX)/share/applications
	install -m 0777 $(PACKAGE) $(DESTDIR)$(PREFIX)/bin
	cp -a doc/*.html $(DESTDIR)$(DOCDIR)/$(PACKAGE)
	cp -f data/tazweb-icon.png \
		$(DESTDIR)$(PREFIX)/share/pixmaps/tazweb.png
	install -m 0644 data/tazweb.desktop \
		$(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f $(PACKAGE)

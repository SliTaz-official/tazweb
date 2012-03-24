# Makefile for TazWeb
#
PREFIX?=/usr
DOCDIR?=$(PREFIX)/share/doc
DESTDIR?=

PACKAGE=tazweb
VERSION=1.5

all:
	gcc src/main.c -o $(PACKAGE) \
		`pkg-config --cflags --libs gtk+-2.0 webkit-1.0`
	@du -sh $(PACKAGE)

install:
	mkdir -p \
		$(DESTDIR)$(DOCDIR)/$(PACKAGE) \
		$(DESTDIR)$(PREFIX)/bin \
		$(DESTDIR)$(PREFIX)/share/tazweb \
		$(DESTDIR)$(PREFIX)/share/pixmaps \
		$(DESTDIR)$(PREFIX)/share/applications
	install -m 0755 $(PACKAGE) $(DESTDIR)$(PREFIX)/bin
	cp -d doc/* $(DESTDIR)$(DOCDIR)/$(PACKAGE)
	install -m 0644 data/tazweb.png \
		$(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 0644 data/tazweb.desktop \
		$(DESTDIR)$(PREFIX)/share/applications
	cp -a data/*.html $(DESTDIR)$(PREFIX)/share/tazweb
	install -m 0644 data/style.css \
		$(DESTDIR)$(PREFIX)/share/tazweb

clean:
	rm -f $(PACKAGE)

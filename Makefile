# Makefile for TazWeb
#
PREFIX?=/usr
DOCDIR?=$(PREFIX)/share/doc
DESTDIR?=

PACKAGE=tazweb
VERSION=1.11
LINGUAS?=$(shell grep -v "^\#" po/LINGUAS)

CC?=gcc

all:
	$(CC) src/tazweb.c -o $(PACKAGE) $(CFLAGS) \
		`pkg-config --cflags --libs gtk+-2.0 webkit-1.0`
	@du -sh $(PACKAGE)

# Next generation
ng:
	$(CC) src/tazweb-ng.c -o $(PACKAGE)-ng $(CFLAGS) \
		`pkg-config --cflags --libs gtk+-2.0 webkit-1.0`
	@du -sh $(PACKAGE)-ng
	
qt:
	cd src && qmake && make
	@du -sh src/$(PACKAGE)-qt

# i18n

pot:
	xgettext -o po/$(PACKAGE).pot -k_ \
		--package-name="TazWeb" \
		--package-version="$(VERSION)" \
		./src/tazweb.c ./lib/helper.sh ./data/tazweb.desktop.in

msgmerge:
	@for l in $(LINGUAS); do \
		echo -n "Updating $$l po file."; \
		msgmerge -U po/$$l.po po/$(PACKAGE).pot; \
	done;

msgfmt:
	@for l in $(LINGUAS); do \
		echo "Compiling $$l mo file..."; \
		mkdir -p po/mo/$$l/LC_MESSAGES; \
		msgfmt -o po/mo/$$l/LC_MESSAGES/$(PACKAGE).mo po/$$l.po; \
	done;

desktop:
	msgfmt --desktop --template=data/tazweb.desktop.in -d po \
		-o data/tazweb.desktop;

install: msgfmt desktop
	mkdir -p \
		$(DESTDIR)$(DOCDIR)/$(PACKAGE) \
		$(DESTDIR)$(PREFIX)/bin \
		$(DESTDIR)$(PREFIX)/lib/tazweb \
		$(DESTDIR)$(PREFIX)/share/tazweb \
		$(DESTDIR)$(PREFIX)/share/icons/hicolor/32x32/apps \
		$(DESTDIR)$(PREFIX)/share/applications
	install -m 0755 $(PACKAGE) $(DESTDIR)$(PREFIX)/bin
	install -m 0755 lib/helper.sh $(DESTDIR)$(PREFIX)/lib/tazweb
	cp -d doc/* $(DESTDIR)$(DOCDIR)/$(PACKAGE)
	install -m 0644 data/tazweb.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/32x32/apps
	install -m 0644 data/tazweb.desktop $(DESTDIR)$(PREFIX)/share/applications
	install -m 0644 data/bookmarks.txt $(DESTDIR)$(PREFIX)/share/tazweb
	mkdir -p $(DESTDIR)$(PREFIX)/share/locale
	cp -a po/mo/* $(DESTDIR)$(PREFIX)/share/locale

clean:
	rm -f $(PACKAGE)
	rm -rf po/mo
	rm -f po/*.mo
	rm -f po/*.*~
	rm -f src/Makefile src/*.o src/tazweb-qt
	rm -f data/*.desktop

help:
	@echo "make [ ng | qt | pot | msgmerge | msgfmt | install | clean ]"

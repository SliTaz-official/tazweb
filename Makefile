# Makefile for TazWeb
#
PREFIX?=/usr
DOCDIR?=$(PREFIX)/share/doc
DESTDIR?=

PACKAGE=tazweb
VERSION=1.6.4
LINGUAS?=fr pt_BR ru

all:
	gcc src/main.c -o $(PACKAGE) \
		`pkg-config --cflags --libs gtk+-2.0 webkit-1.0`
	@du -sh $(PACKAGE)

# i18n

pot:
	xgettext -o po/$(PACKAGE).pot -L C -k_ \
		--package-name="TazWeb" \
		--package-version="$(VERSION)" \
		./src/main.c

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
	mkdir -p $(DESTDIR)$(PREFIX)/share/locale
	cp -a po/mo/* $(DESTDIR)$(PREFIX)/share/locale

clean:
	rm -f $(PACKAGE)
	rm -rf po/mo
	rm -f po/*.mo
	rm -f po/*.*~

help:
	@echo "make [ pot | msgmerge | msgfmt | install | clean ]"

# Makefile for TazWeb
#
PREFIX?=/usr
DOCDIR?=$(PREFIX)/share/doc
DESTDIR?=

PACKAGE=tazweb
VERSION=1.10
LINGUAS?=de fr pt_BR ru vi_VN zh_CN zh_TW

CC?=gcc

all:
	$(CC) src/main.c -o $(PACKAGE) $(CFLAGS) \
		`pkg-config --cflags --libs gtk+-2.0 webkit-1.0`
	@du -sh $(PACKAGE)

qt:
	cd src && qmake && make
	@du -sh src/$(PACKAGE)-qt

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
		$(DESTDIR)/var/www/cgi-bin \
		$(DESTDIR)$(PREFIX)/share/tazweb \
		$(DESTDIR)$(PREFIX)/share/icons/hicolor/32x32/apps \
		$(DESTDIR)$(PREFIX)/share/applications
	install -m 0755 $(PACKAGE) $(DESTDIR)$(PREFIX)/bin
	cp -d doc/* $(DESTDIR)$(DOCDIR)/$(PACKAGE)
	install -m 0644 data/tazweb.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/32x32/apps
	install -m 0644 data/tazweb.desktop $(DESTDIR)$(PREFIX)/share/applications
	install -m 0644 data/bookmarks.txt $(DESTDIR)$(PREFIX)/share/tazweb
	install -m 0755 data/bookmarks.cgi $(DESTDIR)/var/www/cgi-bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/locale
	cp -a po/mo/* $(DESTDIR)$(PREFIX)/share/locale

clean:
	rm -f $(PACKAGE)
	rm -rf po/mo
	rm -f po/*.mo
	rm -f po/*.*~
	rm -f src/Makefile src/*.o src/tazweb-qt

help:
	@echo "make [ pot | msgmerge | msgfmt | install | clean ]"

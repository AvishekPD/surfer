EXTENSION_DIR=$(DESTDIR)/usr/lib/surfer/


ifeq ($(DEBUG), 1)
	CFLAGS += -Wall -g
else
	DEBUG = 0
	CFLAGS += -Wno-deprecated-declarations


#CFLAGS   += -std=c99 -pipe -Wall -fPIC
endif
DDEBUG=-DDEBUG=${DEBUG}

all: surfer adblock.so

surfer: surfer.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< \
		`pkg-config --cflags --libs libnotify gtk+-3.0 glib-2.0 webkit2gtk-4.0` \
		-DEXTENSION_DIR=\"$(EXTENSION_DIR)\" \
		$(DDEBUG) -lm

adblock.so: ext.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -shared -fPIC \
		`pkg-config --cflags --libs libnotify gtk+-3.0 glib-2.0 webkit2gtk-4.0` \
		$(DDEBUG)


install:all
	install -d $(DESTDIR)/usr/share/surfer
	install -d $(DESTDIR)/usr/lib
	install -d $(DESTDIR)/usr/lib/surfer
#	install -m0644 ext.so $(DESTDIR)/usr/lib/surfer/
	install -Dm755 surfer $(DESTDIR)/usr/bin/surfer
	install -Dm644 surfer.desktop $(DESTDIR)/usr/share/applications/surfer.desktop
	install -Dm644 surfer.1 $(DESTDIR)/usr/share/man/man1/surfer.1
	install -Dm644 black.css $(DESTDIR)/usr/share/surfer/black.css
	install -Dm644 icons/24x24/surfer.png $(DESTDIR)/usr/share/icons/hicolor/24x24/surfer.png
	install -Dm644 icons/32x32/surfer.png $(DESTDIR)/usr/share/icons/hicolor/32x32/surfer.png
	install -Dm644 icons/48x48/surfer.png $(DESTDIR)/usr/share/icons/hicolor/48x48/surfer.png
	install -Dm644 icons/72x72/surfer.png $(DESTDIR)/usr/share/icons/hicolor/72x72/surfer.png
	install -Dm644 icons/96x96/surfer.png $(DESTDIR)/usr/share/icons/hicolor/96x96/surfer.png
	install -Dm644 icons/144x144/surfer.png $(DESTDIR)/usr/share/icons/hicolor/144x144/surfer.png
#	install -Dm644 icons/256x256/surfer.png $(DESTDIR)/usr/share/icons/hicolor/256x256/surfer.png
#	install -Dm644 icons/512x512/surfer.png $(DESTDIR)/usr/share/icons/hicolor/512x512/surfer.png
#	install -Dm644 icons/1024x1024/surfer.png $(DESTDIR)/usr/share/icons/hicolor/1024x1024/surfer.png

uninstall:
	$(RM) $(DESTDIR)/usr/bin/surfer
	$(RM) $(DESTDIR)/usr/man/man1/surfer.1
	$(RM) $(DESTDIR)/usr/share/surfer/black.css
	$(RM) $(DESTDIR)/usr/share/applications/surfer.desktop









PREFIX ?= /usr/local
CFLAGS ?= -O3 -g -Wall
LDFLAGS ?= -lm

MANDIR = $(PREFIX)/share/man
MAN1DIR = $(MANDIR)/man1

LIN_CFLAGS = $(CFLAGS)
LIN_LDFLAGS = $(LDFLAGS)

VERSION=0.82.05
VERSIONCODE=$(shell echo $(VERSION) | sed s/\\.//g | sed s/^0// )
# variables for RPM creation
TOPDIR=$(HOME)/rpmbuild
SPECFILE=pktriggercord.spec

# variables for DEB creation
DEBEMAIL="andras.salamon@melda.info"
DEBFULLNAME="Andras Salamon"

# variables for RPM/DEB creation
DESTDIR ?=
ARCH=$(shell uname -m)

#variables for Android
ANDROID=android
ANDROID_DIR = android
ANDROID_ANT_FILE = $(ANDROID_DIR)/build.xml
ANDROID_PROJECT_NAME = PkTriggerCord
APK_FILE = $(PROJECT_NAME)-debug.apk
NDK_BUILD = ndk-build

LIN_GUI_LDFLAGS=$(shell pkg-config --libs gtk+-2.0 gmodule-2.0)
LIN_GUI_CFLAGS=$(CFLAGS) $(shell pkg-config --cflags gtk+-2.0 gmodule-2.0)

default: cli pktriggercord
all: srczip rpm win pktriggercord_commandline.html
cli: pktriggercord-cli

MANS = pktriggercord-cli.1 pktriggercord.1
SRCOBJNAMES = pslr pslr_enum pslr_scsi pslr_lens pslr_model pktriggercord-servermode
OBJS = $(SRCOBJNAMES:=.o)
WIN_DLLS_DIR=win_dlls
SOURCE_PACKAGE_FILES = Makefile Changelog COPYING INSTALL BUGS $(MANS) pentax.rules samsung.rules $(SRCOBJNAMES:=.h) $(SRCOBJNAMES:=.c) pslr_scsi_linux.c pslr_scsi_win.c exiftool_pentax_lens.txt pktriggercord.c pktriggercord-cli.c pktriggercord.ui $(SPECFILE) android_scsi_sg.h
TARDIR = pktriggercord-$(VERSION)
SRCZIP = pkTriggerCord-$(VERSION).src.tar.gz

WINGCC=i686-w64-mingw32-gcc
WINMINGW=/usr/i686-w64-mingw32/sys-root/mingw
WINDIR=$(TARDIR)-win

EXIFTOOL_PENTAXPM=/usr/lib/perl5/vendor_perl/5.20.1/Image/ExifTool/Pentax.pm

pslr.o: pslr_enum.o pslr_scsi.o pslr.c pslr.h

pktriggercord-cli: pktriggercord-cli.c $(OBJS)
	$(CC) $(LIN_CFLAGS) $^ -DVERSION='"$(VERSION)"' -o $@ $(LIN_LDFLAGS) -L.

%.o : %.c %.h
	$(CC) $(LIN_CFLAGS) -fPIC -c $<

pktriggercord: pktriggercord.c $(OBJS)
	$(CC) $(LIN_GUI_CFLAGS) -DVERSION='"$(VERSION)"' -DDATADIR=\"$(PREFIX)/share/pktriggercord\" $? $(LIN_LDFLAGS) -o $@ $(LIN_GUI_LDFLAGS) -L.

install: pktriggercord-cli pktriggercord
	install -d $(DESTDIR)/$(PREFIX)/bin
	install -s -m 0755 pktriggercord-cli $(DESTDIR)/$(PREFIX)/bin/
	(which setcap && setcap CAP_SYS_RAWIO+eip $(DESTDIR)/$(PREFIX)/bin/pktriggercord-cli) || true
	install -d $(DESTDIR)/etc/udev/rules.d
	install -m 0644 pentax.rules $(DESTDIR)/etc/udev/
	install -m 0644 samsung.rules $(DESTDIR)/etc/udev/
	cd $(DESTDIR)/etc/udev/rules.d;\
	ln -sf ../pentax.rules 95_pentax.rules;\
	ln -sf ../samsung.rules 95_samsung.rules
	install -d -m 0755 $(DESTDIR)/$(MAN1DIR)
	install -m 0644 $(MANS) $(DESTDIR)/$(MAN1DIR)
	if [ -e ./pktriggercord ] ; then \
	install -s -m 0755 pktriggercord $(DESTDIR)/$(PREFIX)/bin/; \
	(which setcap && setcap CAP_SYS_RAWIO+eip $(DESTDIR)/$(PREFIX)/bin/pktriggercord) || true; \
	install -d $(DESTDIR)/$(PREFIX)/share/pktriggercord/; \
	install -m 0644 pktriggercord.ui $(DESTDIR)/$(PREFIX)/share/pktriggercord/ ; \
	fi

clean:
	rm -f pktriggercord pktriggercord-cli *.o
	rm -f pktriggercord.exe pktriggercord-cli.exe

uninstall:
	rm -f $(PREFIX)/bin/pktriggercord $(PREFIX)/bin/pktriggercord-cli
	rm -rf $(PREFIX)/share/pktriggercord
	rm -f /etc/udev/pentax.rules
	rm -f /etc/udev/rules.d/95_pentax.rules
	rm -f /etc/udev/samsung.rules
	rm -f /etc/udev/rules.d/95_samsung.rules

srczip: clean
	mkdir -p $(TARDIR)
	cp -r $(SOURCE_PACKAGE_FILES) $(TARDIR)/
	mkdir -p $(TARDIR)/$(WIN_DLLS_DIR)
	cp -r $(WIN_DLLS_DIR)/*.dll $(TARDIR)/$(WIN_DLLS_DIR)/
	mkdir -p $(TARDIR)/debian
	cp -r debian/* $(TARDIR)/debian/
	mkdir -p $(TARDIR)/android/res
	cp -r $(ANDROID_DIR)/res/* $(TARDIR)/$(ANDROID_DIR)/res/
	mkdir -p $(TARDIR)/android/src
	cp -r $(ANDROID_DIR)/src/* $(TARDIR)/$(ANDROID_DIR)/src/
	mkdir -p $(TARDIR)/android/jni
	cp -r $(ANDROID_DIR)/jni/* $(TARDIR)/$(ANDROID_DIR)/jni/
	cp $(ANDROID_DIR)/build.xml $(TARDIR)/$(ANDROID_DIR)/
	cp $(ANDROID_DIR)/ant.properties $(TARDIR)/$(ANDROID_DIR)/
	cp $(ANDROID_DIR)/project.properties $(TARDIR)/$(ANDROID_DIR)/
	cp $(ANDROID_DIR)/proguard-project.txt $(TARDIR)/$(ANDROID_DIR)/
	cp $(ANDROID_DIR)/AndroidManifest.xml $(TARDIR)/$(ANDROID_DIR)/
	tar cf - $(TARDIR) | gzip > $(SRCZIP)
	rm -rf $(TARDIR)

srcrpm: srczip
	install $(SPECFILE) $(TOPDIR)/SPECS/
	install $(SRCZIP) $(TOPDIR)/SOURCES/
	rpmbuild -bs $(SPECFILE)
	cp $(TOPDIR)/SRPMS/pktriggercord-$(VERSION)-1.src.rpm .

rpm: srcrpm
	rpmbuild -ba $(SPECFILE)
	cp $(TOPDIR)/RPMS/$(ARCH)/pktriggercord-$(VERSION)-1.$(ARCH).rpm .

WIN_CFLAGS=$(CFLAGS) -I$(WINMINGW)/include/gtk-2.0/ -I$(WINMINGW)/lib/gtk-2.0/include/ -I$(WINMINGW)/include/atk-1.0/ -I$(WINMINGW)/include/cairo/ -I$(WINMINGW)/include/gdk-pixbuf-2.0/ -I$(WINMINGW)/include/pango-1.0/
WIN_GUI_CFLAGS=$(WIN_CFLAGS) -I$(WINMINGW)/include/glib-2.0 -I$(WINMINGW)/lib/glib-2.0/include
WIN_LDFLAGS=-lgtk-win32-2.0 -lgdk-win32-2.0 -lgdk_pixbuf-2.0 -lgobject-2.0 -lglib-2.0 -lgio-2.0

deb: srczip
	rm -f pktriggercord*orig.tar.gz
	rm -f pktriggercord*debian.tar.gz
	rm -f pktriggercord*armhf*
	rm -rf pktriggercord-$(VERSION)
	tar xvfz pkTriggerCord-$(VERSION).src.tar.gz
	cd pktriggercord-$(VERSION);\
	dh_make -y --single -f ../pkTriggerCord-$(VERSION).src.tar.gz;\
	cp ../debian/* debian/;\
	find debian/ -size 0 | xargs rm -f;\
	dpkg-buildpackage -us -uc
	rm -rf pktriggercord-$(VERSION)

# Remote deb creation on Raspberry PI
# address, dir hardwired
remotedeb:
	ssh pi@raspberrypi "rm -rf /tmp/pktriggercord && cd /tmp && git clone --depth 1 https://github.com/asalamon74/pktriggercord.git && cd pktriggercord && make clean deb"
	scp pi@raspberrypi:/tmp/pktriggercord/pktriggercord_*.deb .


# converting lens info from exiftool
exiftool_pentax_lens.txt: $(EXIFTOOL_PENTAXPM)
	cat $(EXIFTOOL_PENTAXPM) | sed -n '/%pentaxLensTypes\ =/,/%pentaxModelID/p' | grep -v '^\s*#' | sed -e "s/[ ]*'\([0-9]\) \([0-9]\{1,3\}\)' => '\(.*\)',.*/{\1, \2, \"\3\"},/g;tx;d;:x" > $@

pktriggercord_commandline.html: pktriggercord-cli.1
	groff $< -man -Thtml -mwww -P "-lr" > $@

# Windows cross-compile
win: clean pktriggercord_commandline.html
	$(foreach srcfile, $(SRCOBJNAMES:=.c), $(WINGCC) $(WIN_CFLAGS) -c $(srcfile);)
	$(WINGCC) -mms-bitfields -DVERSION='"$(VERSION)"'  pktriggercord-cli.c $(OBJS) -o pktriggercord-cli.exe $(WIN_CFLAGS) $(WIN_LDFLAGS) -L.
	$(WINGCC) -mms-bitfields -DVERSION='"$(VERSION)"' -DDATADIR=\".\" pktriggercord.c $(OBJS) -o pktriggercord.exe $(WIN_GUI_CFLAGS) $(WIN_LDFLAGS) -L.
	mkdir -p $(WINDIR)
	cp pktriggercord.exe pktriggercord-cli.exe pktriggercord.ui Changelog COPYING pktriggercord_commandline.html $(WINDIR)
	cp $(WIN_DLLS_DIR)/*.dll $(WINDIR)
	rm -f $(WINDIR).zip
	zip -rj $(WINDIR).zip $(WINDIR)
	rm -r $(WINDIR)

androidcreate:
	$(ANDROID) create project \
	--path $(ANDROID_DIR) \
	--target android-12 \
	--name $(ANDROID_PROJECT_NAME) \
	--package info.melda.sala.pktriggercord \
	--activity MainActivity
	mkdir $(ANDROID_DIR)/jni
	ln -s ../.. $(ANDROID_DIR)/jni/src

$(ANDROID_DIR)/build.xml $(ANDROID_DIR)/local.properties:
	$(ANDROID) update project --path $(ANDROID_DIR) --target android-12

androidcli: $(ANDROID_DIR)/build.xml $(ANDROID_DIR)/local.properties
	VERSION=$(VERSION) NDK_PROJECT_PATH=$(ANDROID_DIR) NDK_DEBUG=1 $(NDK_BUILD)

androidclean:
	VERSION=$(VERSION) NDK_PROJECT_PATH=$(ANDROID_DIR) NDK_DEBUG=1 $(NDK_BUILD) clean
	ant -f $(ANDROID_ANT_FILE) clean
	rm -rf $(ANDROID_DIR)/assets
	rm -rf $(ANDROID_DIR)/libs

androidver:
	sed -i s/android:versionName=\".*\"/android:versionName=\"$(VERSION)\"/ $(ANDROID_DIR)/AndroidManifest.xml
	sed -i s/android:versionCode=\".*\"/android:versionCode=\"$(VERSIONCODE)\"/ $(ANDROID_DIR)/AndroidManifest.xml

android: androidcli androidver $(ANDROID_DIR)/build.xml
	mkdir -p $(ANDROID_DIR)/assets
	cp $(ANDROID_DIR)/libs/armeabi/pktriggercord-cli $(ANDROID_DIR)/assets
	ant "-Djava.compilerargs=-Xlint:unchecked -Xlint:deprecation" -f $(ANDROID_ANT_FILE) debug
	cp $(ANDROID_DIR)/bin/$(ANDROID_PROJECT_NAME)-debug.apk $(ANDROID_PROJECT_NAME)-$(VERSION)-debug.apk
	echo "android build is EXPERIMENTAL. Use it at your own risk"

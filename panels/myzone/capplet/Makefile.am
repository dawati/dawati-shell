if ENABLE_CAPPLET
ccmodulesdir = $(EXTENSIONSDIR)
ccmodules_LTLIBRARIES = libmyzone.la

libmyzone_la_SOURCES = myzone-cc-panel.c myzone-cc-panel.h gconf-bridge.c gconf-bridge.h
libmyzone_la_LIBADD = $(CAPPLET_LIBS)
libmyzone_la_LDFLAGS = -avoid-version -module
libmyzone_la_CFLAGS = $(CAPPLET_CFLAGS) \
		      -DUI_DIR=\"$(pkgdatadir)\" \
		      -DLOCALEDIR=\""$(datadir)/locale"\"
endif

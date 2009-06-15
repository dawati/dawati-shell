
pkglib_LTLIBRARIES = libmnb-gtk.la libmnb-clutter.la

libmnb_includedir = $(includedir)/libmnb/mnb
libmnb_include_HEADERS = \
	mnb-panel-client.h \
	mnb-panel-clutter.h \
	mnb-panel-common.h \
	mnb-panel-gtk.h \
	$(top_srcdir)/src/mnb-entry.h

libmnb_clutter_la_CFLAGS = $(LIBMNB_CLUTTER_CFLAGS)

libmnb_clutter_la_LDFLAGS = $(LIBMNB_CLUTTER_LIBS)

libmnb_clutter_la_SOURCES = \
	mnb-panel-client.c \
	mnb-panel-clutter.c \
	penge-stubs.c	\
	$(top_srcdir)/src/marshal.c	\
	$(top_srcdir)/src/mnb-entry.c

libmnb_gtk_la_CFLAGS = $(LIBMNB_GTK_CFLAGS)

libmnb_gtk_la_LDFLAGS = $(LIBMNB_GTK_LIBS)

libmnb_gtk_la_SOURCES = \
	mnb-panel-client.c \
	mnb-panel-gtk.c \
	$(top_srcdir)/src/marshal.c


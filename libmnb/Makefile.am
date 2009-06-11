
pkglib_LTLIBRARIES = libmnb-gtk.la libmnb-clutter.la

libmnb_common = mnb-panel-client.c	\
		mnb-panel-common.h	\
		../src/marshal.c

libmnbclutter_headers = mnb-panel-client.h ../src/mnb-entry.h
libmnbgtk_headers = mnb-panel-gtk.h

libmnb_includedir = $(includedir)/mutter-moblin
libmnb_include_HEADERS = mnb-panel-client.h	\
			 $(libmnbgtk_headers)	\
			 $(libmnbclutter_headers)

libmnb_gtk_la_SOURCES = \
			$(libmnbgtk_headers) 	\
			$(libmnb_common)	\
			mnb-panel-gtk.c

libmnb_gtk_la_CFLAGS = -Wall $(LIBMNB_CFLAGS) $(GTK_CFLAGS) -I../penge
libmnb_gtk_la_LDFLAGS = $(LIBMNB_LIBS) $(GTK_LIBS)

libmnb_clutter_la_SOURCES = \
			$(libmnbclutter_headers)	\
			$(libmnb_common)		\
			mnb-panel-clutter.c		\
			../src/mnb-entry.c		\
			penge-stubs.c

libmnb_clutter_la_CFLAGS = -Wall $(LIBMNB_CFLAGS) $(NBTK_CFLAGS) -I../penge
libmnb_clutter_la_LDFLAGS = $(LIBMNB_LIBS) $(NBTK_LIBS) ../penge/penge/libpenge.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mnb-gtk.pc mnb-clutter.pc

EXTRA_DIST=mnb-gtk.pc.in mnb-clutter.pc.in

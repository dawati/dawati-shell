
pkglib_LTLIBRARIES = libmnb-gtk.la libmnb-clutter.la

libmnb_includedir = $(includedir)/mutter-moblin
libmnb_include_HEADERS = mnb-panel-client.h	\
			 mnb-panel-gtk.h	\
			 mnb-panel-clutter.h

libmnb_gtk_la_SOURCES = \
			$(libmnbgtk_headers) 	\
			mnb-panel-client.c 	\
			mnb-panel-gtk.c		\
			../src/marshal.c

libmnb_gtk_la_CFLAGS = -Wall $(LIBMNB_CFLAGS) $(GTK_CFLAGS)
libmnb_gtk_la_LDFLAGS = $(LIBMNB_LIBS) $(GTK_LIBS)

libmnb_clutter_la_SOURCES = \
			$(libmnbclutter_headers) 	\
			mnb-panel-client.c 		\
			mnb-panel-clutter.c		\
			../src/marshal.c

libmnb_clutter_la_CFLAGS = -Wall $(LIBMNB_CFLAGS) $(NBTK_CFLAGS)
libmnb_clutter_la_LDFLAGS = $(LIBMNB_LIBS) $(NBTK_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mnb-gtk.pc mnb-clutter.pc

EXTRA_DIST=mnb-gtk.pc.in mnb-clutter.pc.in

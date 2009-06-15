
pkglib_LTLIBRARIES = libmoblin-panel-gtk.la libmoblin-panel-clutter.la

libmoblin_panel_includedir = $(includedir)/libmoblin-panel/moblin-panel
libmoblin_panel_include_HEADERS = \
	mpl-entry.h \
	mpl-panel-client.h \
	mpl-panel-clutter.h \
	mpl-panel-common.h \
	mpl-panel-gtk.h

libmoblin_panel_clutter_la_CFLAGS = $(LIBMPL_CLUTTER_CFLAGS)

libmoblin_panel_clutter_la_LDFLAGS = $(LIBMPL_CLUTTER_LIBS)

libmoblin_panel_clutter_la_SOURCES = \
	mpl-entry.c \
	mpl-panel-client.c \
	mpl-panel-clutter.c \
	penge-stubs.c	\
	$(top_srcdir)/src/marshal.c

libmoblin_panel_gtk_la_CFLAGS = $(LIBMPL_GTK_CFLAGS)

libmoblin_panel_gtk_la_LDFLAGS = $(LIBMPL_GTK_LIBS)

libmoblin_panel_gtk_la_SOURCES = \
	mpl-panel-client.c \
	mpl-panel-gtk.c \
	$(top_srcdir)/src/marshal.c


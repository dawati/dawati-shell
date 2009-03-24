noinst_LTLIBRARIES = \
		     libdalston.la

libdalston_la_SOURCES = dalston-battery-monitor.c \
			dalston-battery-monitor.h \
			dalston-brightness-manager.c \
			dalston-brightness-manager.h \
			dalston-brightness-slider.c \
			dalston-brightness-slider.h

libdalston_la_CFLAGS = -I$(top_srcdir) \
		       $(DBUS_GLIB_CFLAGS) \
		       $(GLIB_CFLAGS) \
		       $(GTK_CFLAGS)

libdalston_la_LIBADD = $(top_builddir)/libhal-glib/libhal-glib.la \
		       $(top_builddir)/libhal-panel-glib/libhal-panel-glib.la \
		       $(DBUS_GLIB_LIBS) \
		       $(GLIB_LIBS) \
		       $(GTK_LIBS)


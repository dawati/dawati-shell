noinst_LTLIBRARIES = \
		     libdalston.la

libdalston_la_SOURCES = dalston-battery-monitor.c \
			dalston-battery-monitor.h \
			dalston-brightness-manager.c \
			dalston-brightness-manager.h \
			dalston-brightness-slider.c \
			dalston-brightness-slider.h \
			dalston-power-applet.h \
			dalston-power-applet.c \
			dalston-volume-status-icon.c \
			dalston-volume-status-icon.h \
			dalston-volume-slider.c \
			dalston-volume-slider.h \
			dalston-volume-pane.c \
			dalston-volume-pane.h \
			dalston-volume-applet.c \
			dalston-volume-applet.h

libdalston_la_CFLAGS = -I$(top_srcdir) \
		       $(DBUS_GLIB_CFLAGS) \
		       $(GLIB_CFLAGS) \
		       $(GTK_CFLAGS) \
		       $(PULSE_CFLAGS) \
		       $(CANBERRA_CFLAGS) \
		       -DPKG_DATA_DIR=\"$(pkgdatadir)\"

libdalston_la_LIBADD = $(top_builddir)/libhal-glib/libhal-glib.la \
		       $(top_builddir)/libhal-panel-glib/libhal-panel-glib.la \
		       $(top_builddir)/libhal-power-glib/libhal-power-glib.la \
		       $(top_builddir)/libgvc/libgvc.la
		       $(DBUS_GLIB_LIBS) \
		       $(GLIB_LIBS) \
		       $(GTK_LIBS) \
		       $(PULSE_LIBS) \
		       $(CANBERRA_LIBS)


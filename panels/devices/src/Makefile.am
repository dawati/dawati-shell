bin_PROGRAMS = dalston-volume-applet dalston-power-applet

dalston_volume_applet_CFLAGS = -I$(top_srcdir) \
			       $(DBUS_GLIB_CFLAGS) \
			       $(GLIB_CFLAGS) \
			       $(GTK_CFLAGS) \
			       $(PULSE_CFLAGS)\
			       $(CANBERRA_CFLAGS)
dalston_volume_applet_LDADD = $(top_builddir)/dalston/libdalston.la \
			      $(top_builddir)/libgvc/libgvc.la \
			      $(top_builddir)/libhal-glib/libhal-glib.la \
			      $(top_builddir)/libhal-panel-glib/libhal-panel-glib.la \
			      $(DBUS_GLIB_LIBS) \
			      $(GLIB_LIBS) \
			      $(GTK_LIBS) \
			      $(CANBERRA_LIBS)
dalston_volume_applet_SOURCES = dalston-volume-applet.c moblin-netbook-system-tray.h

dalston_power_applet_CFLAGS = -I$(top_srcdir) \
			      $(DBUS_GLIB_CFLAGS) \
			      $(GLIB_CFLAGS) \
			      $(GTK_CFLAGS) \
			      $(PULSE_CFLAGS)\
			      $(CANBERRA_CFLAGS)
dalston_power_applet_LDADD = $(top_builddir)/dalston/libdalston.la \
			     $(top_builddir)/libgvc/libgvc.la \
			     $(top_builddir)/libhal-glib/libhal-glib.la \
			     $(top_builddir)/libhal-panel-glib/libhal-panel-glib.la \
			     $(DBUS_GLIB_LIBS) \
			     $(GLIB_LIBS) \
			     $(GTK_LIBS) \
			     $(CANBERRA_LIBS)
dalston_power_applet_SOURCES = dalston-power-applet.c moblin-netbook-system-tray.h

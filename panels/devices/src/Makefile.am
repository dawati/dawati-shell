libexec_PROGRAMS = dalston-volume-applet dalston-power-applet

if WITH_SETTINGS_CAPPLET
libexec_PROGRAMS += dalston-power-settings-capplet
endif

dalston_volume_applet_CFLAGS = -I$(top_srcdir) \
			       $(DBUS_GLIB_CFLAGS) \
			       $(GLIB_CFLAGS) \
			       $(GTK_CFLAGS) \
			       $(PULSE_CFLAGS)\
			       $(CANBERRA_CFLAGS) \
			       $(NBTK_GTK_CFLAGS) \
			       $(LIBNOTIFY_CFLAGS) \
			       $(MPL_CFLAGS) \
			      -DLOCALEDIR=\""$(localedir)"\" \
			      -DPKG_DATADIR=\""$(pkgdatadir)"\"
dalston_volume_applet_LDADD = $(top_builddir)/dalston/libdalston.la \
			      $(top_builddir)/libgvc/libgvc.la \
			      $(top_builddir)/libhal-glib/libhal-glib.la \
			      $(top_builddir)/libhal-panel-glib/libhal-panel-glib.la \
			      $(DBUS_GLIB_LIBS) \
			      $(GLIB_LIBS) \
			      $(GTK_LIBS) \
			      $(CANBERRA_LIBS) \
			      $(NBTK_GTK_LIBS) \
			      $(LIBNOTIFY_LIBS) \
			      $(GCONF_LIBS) \
			      $(MPL_LIBS)
dalston_volume_applet_SOURCES = dalston-volume-applet.c moblin-netbook-system-tray.h

dalston_power_applet_CFLAGS = -I$(top_srcdir) \
			      $(DBUS_GLIB_CFLAGS) \
			      $(GLIB_CFLAGS) \
			      $(GTK_CFLAGS) \
			      $(PULSE_CFLAGS)\
			      $(CANBERRA_CFLAGS) \
			      $(NBTK_GTK_CFLAGS) \
			      $(LIBNOTIFY_CFLAGS) \
			      $(MPL_CFLAGS) \
			     -DLOCALEDIR=\""$(localedir)"\" \
			     -DPKG_DATADIR=\""$(pkgdatadir)"\"
dalston_power_applet_LDADD = $(top_builddir)/dalston/libdalston.la \
			     $(top_builddir)/libgvc/libgvc.la \
			     $(top_builddir)/libhal-glib/libhal-glib.la \
			     $(top_builddir)/libhal-panel-glib/libhal-panel-glib.la \
			     $(top_builddir)/libhal-power-glib/libhal-power-glib.la \
			     $(top_builddir)/libegg-idletime/libegg-idletime.la \
			     $(DBUS_GLIB_LIBS) \
			     $(GLIB_LIBS) \
			     $(GTK_LIBS) \
			     $(CANBERRA_LIBS) \
			     $(NBTK_GTK_LIBS) \
			     $(LIBNOTIFY_LIBS) \
			     $(MPL_LIBS) \
			     $(GCONF_LIBS)
dalston_power_applet_SOURCES = dalston-power-applet.c moblin-netbook-system-tray.h

if WITH_SETTINGS_CAPPLET
dalston_power_settings_capplet_CFLAGS = -I$(top_srcdir) \
					$(GLIB_CFLAGS) \
					$(GTK_CFLAGS) \
					$(GCONF_CFLAGS)
dalston_power_settings_capplet_LDADD =  $(GLIB_LIBS) \
					$(GTK_LIBS) \
					$(GCONF_LIBS)
dalston_power_settings_capplet_SOURCES = dalston-power-settings-capplet.c
endif


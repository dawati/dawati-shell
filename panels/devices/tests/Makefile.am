noinst_PROGRAMS = test-battery-monitor test-brightness-manager test-brightness-slider test-gvc test-power

AM_CFLAGS = -I$(top_srcdir) \
	    $(DBUS_GLIB_CFLAGS) $(GLIB_CFLAGS) $(GTK_CFLAGS) $(PULSE_CFLAGS) $(CANBERRA_CFLAGS)
test_battery_monitor_LDADD = $(top_builddir)/dalston/libdalston.la \
	     $(top_builddir)/libhal-glib/libhal-glib.la \
	     $(DBUS_GLIB_LIBS) $(GLIB_LIBS)
test_battery_monitor_SOURCES = test-battery-monitor.c
test_brightness_manager_LDADD = $(top_builddir)/dalston/libdalston.la \
	     $(top_builddir)/libhal-glib/libhal-glib.la \
	     $(top_builddir)/libhal-panel-glib/libhal-panel-glib.la \
	     $(DBUS_GLIB_LIBS) $(GLIB_LIBS)
test_brightness_manager_SOURCES = test-brightness-manager.c
test_brightness_slider_LDADD = $(top_builddir)/dalston/libdalston.la \
	     $(top_builddir)/libhal-glib/libhal-glib.la \
	     $(top_builddir)/libhal-panel-glib/libhal-panel-glib.la \
	     $(DBUS_GLIB_LIBS) $(GLIB_LIBS) $(GTK_LIBS)
test_brightness_slider_SOURCES = test-brightness-slider.c
test_gvc_SOURCES = test-gvc.c
test_gvc_LDADD = $(top_builddir)/libgvc/libgvc.la \
		 $(PULSE_LIBS) $(GLIB_LIBS)
test_power_SOURCES = test-power.c
test_power_LDADD = $(top_builddir)/libhal-power-glib/libhal-power-glib.la \
		   $(DBUS_GLIB_LIBS) $(GLIB_LIBS)

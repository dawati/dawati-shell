noinst_PROGRAMS = test-battery-monitor

AM_CFLAGS = -I$(top_srcdir) \
	    $(DBUS_GLIB_CFLAGS) $(GLIB_CFLAGS)
test_battery_monitor_LDADD = $(top_builddir)/dalston/libdalston.la \
	     $(top_builddir)/libhal-glib/libhal-glib.la \
	     $(DBUS_GLIB_LIBS) $(GLIB_LIBS)
test_battery_monitor_SOURCES = test-battery-monitor.c

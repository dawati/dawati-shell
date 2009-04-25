bin_PROGRAMS = carrick-connection-panel

carrick_connection_panel_SOURCES = \
		main.c \
		$(NULL)

INCLUDES = -I$(top_srcdir)/connman-common

carrick_connection_panel_CFLAGS = -I$(top_srcdir) \
				    $(GTK_CFLAGS) \
				    $(DBUS_CFLAGS) \
				    $(CONNMAN_CFLAGS) \
				    -DPKG_DATA_DIR=\"$(pkgdatadir)\" -Wall

carrick_connection_panel_LDADD= $(top_builddir)/mux/libmux.la \
				$(top_builddir)/carrick/libcarrick.la \
				$(GTK_LIBS) $(DBUS_LIBS) $(CONNMAN_LIBS)

-include $(top_srcdir)/git.mk

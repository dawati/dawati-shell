bin_PROGRAMS = connection-panel

connection_panel_SOURCES = \
		main.c \
		$(NULL)

INCLUDES = -I$(top_srcdir)/connman-common

connection_panel_CFLAGS = -I$(top_srcdir) \
			$(GTK_CFLAGS) \
			-DPKG_DATADIR=\"$(pkgdatadir)\" -Wall

connection_panel_LDADD= $(top_builddir)/mux/libmux.la \
			$(top_builddir)/connman-common/libcommon.a \
			$(GTK_LIBS) $(DBUS_LIBS)

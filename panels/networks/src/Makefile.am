libexec_PROGRAMS = carrick-connection-panel

carrick_connection_panel_SOURCES = carrick.c

INCLUDES = -I$(top_srcdir)/connman-common

carrick_connection_panel_CFLAGS = -I$(top_srcdir) \
                                    $(GTK_CFLAGS) \
                                    $(DBUS_CFLAGS) \
                                    $(CONNMAN_CFLAGS) \
                                    $(MPL_CFLAGS) \
                                    -DLOCALEDIR=\""$(localedir)"\" \
                                    -DPKG_DATA_DIR=\"$(pkgdatadir)\" -Wall

carrick_connection_panel_LDADD= $(top_builddir)/carrick/libcarrick.la \
                                $(GTK_LIBS) $(DBUS_LIBS) $(CONNMAN_LIBS) $(MPL_LIBS)

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.network.service.in
service_DATA =     org.moblin.UX.Shell.Panels.network.service

EXTRA_DIST = $(service_in_files)

CLEANFILES = $(service_DATA)
DISTCLEANFILES = $(service_DATA)

%.service: %.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

-include $(top_srcdir)/git.mk

libexec_PROGRAMS = carrick-connection-panel

carrick_connection_panel_SOURCES = carrick.c

INCLUDES = -I$(srcdir)/../connman-common

carrick_connection_panel_CFLAGS = \
	$(LIBMPL_CFLAGS) \
	$(PANEL_NETWORKS_CFLAGS) \
	-I$(srcdir)/../ \
	-DTHEMEDIR=\"$(MUTTER_DAWATI_THEME_DIR)/networks\" \
	-DLOCALEDIR=\""$(localedir)"\" \
	-DPKG_DATA_DIR=\"$(pkgdatadir)\" \
	-Wall

carrick_connection_panel_LDADD = \
	$(LIBMPL_LIBS) \
	$(PANEL_NETWORKS_LIBS) \
	$(top_builddir)/carrick/libcarrick.la \
	$(NULL)

%.service: %.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.network.service.in
service_DATA = com.dawati.UX.Shell.Panels.network.service

EXTRA_DIST = $(service_in_files)
CLEANFILES = $(service_DATA)
DISTCLEANFILES = $(service_DATA)


-include $(top_srcdir)/git.mk

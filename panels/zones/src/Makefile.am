QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(ZONES_CFLAGS) \
	$(MPL_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DSTYLEDIR=\"$(pkgdatadir)/style\" \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DDATADIR=\"$(datadir)\"

libexec_PROGRAMS = dawati-panel-zones

dawati_panel_zones_LDADD = $(ZONES_LIBS) $(MPL_LIBS)

dawati_panel_zones_SOURCES = \
			     penge-magic-texture.c \
			     penge-magic-texture.h \
			     sw-overview.c \
			     sw-overview.h \
			     sw-window.c \
			     sw-window.h \
			     sw-zone.c \
			     sw-zone.h \
			     main.c

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.zones.service.in
service_DATA = com.dawati.UX.Shell.Panels.zones.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = com.dawati.UX.Shell.Panels.zones.service

com.dawati.UX.Shell.Panels.zones.service: com.dawati.UX.Shell.Panels.zones.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

-include $(top_srcdir)/git.mk

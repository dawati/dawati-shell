QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PANEL_ZONES_CFLAGS) \
	-DDAWATI_STYLE_DIR=\"$(DAWATI_STYLE_DIR)\"
	$(NULL)

libexec_PROGRAMS = dawati-panel-zones

dawati_panel_zones_LDADD = \
	$(PANEL_ZONES_LIBS) \
	-lm \
	$(NULL)

dawati_panel_zones_SOURCES = \
	main.c \
	$(NULL)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.zones.service.in
service_DATA = com.dawati.UX.Shell.Panels.zones.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = com.dawati.UX.Shell.Panels.zones.service

com.dawati.UX.Shell.Panels.zones.service: com.dawati.UX.Shell.Panels.zones.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

-include $(top_srcdir)/git.mk

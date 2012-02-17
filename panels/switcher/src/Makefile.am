QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PANEL_SWITCHER_CFLAGS) \
	$(NULL)

libexec_PROGRAMS = dawati-panel-switcher

dawati_panel_switcher_LDADD = \
	$(PANEL_SWITCHER_LIBS) \
	-lm \
	$(NULL)

dawati_panel_switcher_SOURCES = \
	main.c \
	$(NULL)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.switcher.service.in
service_DATA = com.dawati.UX.Shell.Panels.switcher.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = com.dawati.UX.Shell.Panels.switcher.service

com.dawati.UX.Shell.Panels.switcher.service: com.dawati.UX.Shell.Panels.switcher.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

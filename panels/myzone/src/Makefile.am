QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PANEL_MYZONE_CFLAGS) \
	$(PENGE_CFLAGS) \
	-I$(top_srcdir) \
	\
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	$(NULL)

libexec_PROGRAMS = dawati-panel-myzone

dawati_panel_myzone_LDADD = \
	$(PANEL_MYZONE_LIBS) \
	$(PENGE_LIBS) \
	$(top_builddir)/penge/libpenge.la \
	$(NULL)

dawati_panel_myzone_SOURCES = \
	dawati-panel-myzone.c \
	$(NULL)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.myzone.service.in
service_DATA = com.dawati.UX.Shell.Panels.myzone.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = com.dawati.UX.Shell.Panels.myzone.service

com.dawati.UX.Shell.Panels.myzone.service: com.dawati.UX.Shell.Panels.myzone.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

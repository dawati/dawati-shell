QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

INCLUDES = \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\"

sw-marshals.c: sw-marshals.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --body --prefix=sw_marshal $< > $@

sw-marshals.h: sw-marshals.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --header --prefix=sw_marshal $< > $@


libexec_PROGRAMS = dawati-panel-status
lib_LTLIBRARIES = libdawati-panel-status.la

BUILT_SOURCES = sw-marshals.c sw-marshals.h

dawati_panel_status_CFLAGS = \
	$(PANEL_STATUS_CFLAGS) \
	$(NM_CFLAGS) \
	$(NULL)
dawati_panel_status_LDADD = \
	$(PANEL_STATUS_LIBS) \
	$(NM_LIBS) \
	./libdawati-panel-status.la \
	$(NULL)

dawati_panel_status_SOURCES = \
	dawati-panel-status.c \
	$(NULL)

libdawati_panel_status_ladir = $(pkgincludedir)
libdawati_panel_status_la_HEADERS = \
	penge-magic-texture.h \
	penge-clickable-label.h \
	mps-tweet-card.h \
	mps-view-bridge.h \
	mps-feed-pane.h \
	mps-feed-switcher.h \
	mps-geotag-pane.h \
	sw-online.h \
	mps-module.h \
	$(NULL)

libdawati_panel_status_la_SOURCES = \
	penge-magic-texture.c \
	penge-clickable-label.c \
	mps-tweet-card.c \
	mps-view-bridge.c \
	mps-feed-pane.c \
	mps-feed-switcher.c \
	mps-geotag-pane.c \
	sw-online.c \
	$(libdawati_panel_status_la_HEADERS) \
	$(NULL)

libdawati_panel_status_la_CFLAGS = \
	$(PANEL_STATUS_CFLAGS) \
	$(NM_CFLAGS) \
	-DSERVICES_MODULES_DIR=\"$(servicesdir)\" \
	$(NULL)
libdawati_panel_status_la_LIBADD = \
	$(PANEL_STATUS_LIBS) \
	$(NM_LIBS) \
	$(NULL)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.status.service.in
service_DATA = com.dawati.UX.Shell.Panels.status.service

EXTRA_DIST = $(service_in_files) sw-marshals.list

CLEANFILES = stamp-mnb-status-marshal.h

DISTCLEANFILES = $(service_DATA) $(BUILT_SOURCES)

com.dawati.UX.Shell.Panels.status.service: $(service_in_files) Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

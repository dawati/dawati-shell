QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

INCLUDES = \
	-DPREFIX=\"$(prefix)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\"

sw-marshals.c: sw-marshals.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --body --prefix=sw_marshal $< > $@

sw-marshals.h: sw-marshals.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --header --prefix=sw_marshal $< > $@


libexec_PROGRAMS = meego-panel-status
BUILT_SOURCES = sw-marshals.c sw-marshals.h

meego_panel_status_CPPFLAGS = $(STATUS_CFLAGS) $(MPL_CFLAGS) $(NM_CFLAGS)
meego_panel_status_LDADD    = $(STATUS_LIBS) $(MPL_LIBS) $(NM_LIBS)
meego_panel_status_SOURCES  = 	\
	meego-panel-status.c \
	penge-magic-texture.h \
	penge-magic-texture.c \
	penge-clickable-label.c \
	penge-clickable-label.h \
	mps-tweet-card.h \
	mps-tweet-card.c \
	mps-view-bridge.c \
	mps-view-bridge.h \
	mps-feed-pane.c \
	mps-feed-pane.h \
	mps-feed-switcher.c \
	mps-feed-switcher.h \
	mps-geotag-pane.c \
	mps-geotag-pane.h \
	sw-online.c \
	sw-online.h

servicedir = $(datadir)/dbus-1/services
service_in_files = com.meego.UX.Shell.Panels.status.service.in
service_DATA = com.meego.UX.Shell.Panels.status.service

EXTRA_DIST = $(service_in_files) sw-marshals.list

CLEANFILES = stamp-mnb-status-marshal.h

DISTCLEANFILES = $(service_DATA) $(BUILT_SOURCES)

com.meego.UX.Shell.Panels.status.service: $(service_in_files) Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

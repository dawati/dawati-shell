QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

INCLUDES = \
	-DG_LOG_DOMAIN=\"Mutter-Moblin-Status\" \
	-DPREFIX=\"$(prefix)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\"

sw-marshals.c: sw-marshals.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --body --prefix=sw_marshal $< > $@

sw-marshals.h: sw-marshals.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --header --prefix=sw_marshal $< > $@


libexec_PROGRAMS = moblin-panel-status
BUILT_SOURCES = sw-marshals.c sw-marshals.h

moblin_panel_status_CPPFLAGS = $(STATUS_CFLAGS) $(MPL_CFLAGS) $(NM_CFLAGS)
moblin_panel_status_LDADD    = $(STATUS_LIBS) $(MPL_LIBS) $(NM_LIBS)
moblin_panel_status_SOURCES  = 	\
	moblin-panel-status.c \
	penge-magic-texture.h \
	penge-magic-texture.c \
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
service_in_files = org.moblin.UX.Shell.Panels.status.service.in
service_DATA = org.moblin.UX.Shell.Panels.status.service

EXTRA_DIST = $(service_in_files) sw-marshals.list

CLEANFILES = stamp-mnb-status-marshal.h

DISTCLEANFILES = $(service_DATA) $(BUILT_SOURCES)

org.moblin.UX.Shell.Panels.status.service: $(service_in_files) Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

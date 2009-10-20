QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

INCLUDES = \
	-DG_LOG_DOMAIN=\"Mutter-Moblin-Status\" \
	-DPREFIX=\"$(prefix)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\"

mnb-status-marshal.h: stamp-mnb-status-marshal.h
	@true
stamp-mnb-status-marshal.h: mnb-status-marshal.list
	$(QUIET_GEN)$(GLIB_GENMARSHAL) \
		--prefix=mnb_status_marshal \
		--header \
	$(srcdir)/mnb-status-marshal.list > xgen-mh \
	&& (cmp -s xgen-mh mnb-status-marshal.h || cp -f xgen-mh mnb-status-marshal.h ) \
	&& rm -f xgen-mh \
	&& echo timestamp > $(@F)

mnb-status-marshal.c: mnb-status-marshal.h
	$(QUIET_GEN)( echo "#include \"mnb-status-marshal.h\"" ; \
	$(GLIB_GENMARSHAL) \
		--prefix=mnb_status_marshal \
		--body \
	$(srcdir)/mnb-status-marshal.list ) > xgen-mc \
	&& cp -f xgen-mc mnb-status-marshal.c \
	&& rm -f xgen-mc

libexec_PROGRAMS = moblin-panel-status

BUILT_SOURCES = mnb-status-marshal.h mnb-status-marshal.c

moblin_panel_status_CPPFLAGS = $(STATUS_CFLAGS) $(MPL_CFLAGS) $(NM_CFLAGS)
moblin_panel_status_LDADD    = $(STATUS_LIBS) $(MPL_LIBS) $(NM_LIBS)
moblin_panel_status_SOURCES  = 	\
	$(BUILT_SOURCES)	\
	mnb-im-status-row.h	\
	mnb-im-status-row.c	\
	mnb-web-status-entry.h	\
	mnb-web-status-entry.c	\
	mnb-web-status-row.h	\
	mnb-web-status-row.c	\
	moblin-panel-status.c \
	penge-magic-texture.h \
	penge-magic-texture.c \
	mojito-online.h \
	mojito-online.c \
	mnb-launcher-button.h \
	mnb-launcher-button.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.status.service.in
service_DATA = org.moblin.UX.Shell.Panels.status.service

EXTRA_DIST = $(service_in_files) mnb-status-marshal.list

CLEANFILES = stamp-mnb-status-marshal.h

DISTCLEANFILES = $(service_DATA) $(BUILT_SOURCES)

org.moblin.UX.Shell.Panels.status.service: $(service_in_files) Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

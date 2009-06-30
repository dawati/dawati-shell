
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PASTEBOARD_CFLAGS) \
	-DPKGDATADIR=\"$(datadir)/moblin-panel-pasteboard\"

# FIXME use this once split out
#	-DPKGDATADIR=\"$(pkgdatadir)\"

libexec_PROGRAMS = moblin-panel-pasteboard

moblin_panel_pasteboard_LDADD = $(PASTEBOARD_LIBS)

moblin_panel_pasteboard_SOURCES = \
	$(BUILT_SOURCES) \
	mnb-clipboard-item.c \
	mnb-clipboard-item.h \
	mnb-clipboard-store.c \
	mnb-clipboard-store.h \
	mnb-clipboard-view.c \
	mnb-clipboard-view.h \
	moblin-netbook-pasteboard.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.pasteboard.service.in
service_DATA = org.moblin.UX.Shell.Panels.pasteboard.service

BUILT_SOURCES = \
	mnb-pasteboard-marshal.c \
	mnb-pasteboard-marshal.h

CLEANFILES = \
	$(BUILT_SOURCES) \
	stamp-mnb-pasteboard-marshal.h

DISTCLEANFILES = \
	org.moblin.UX.Shell.Panels.pasteboard.service

EXTRA_DIST = \
	$(service_in_files) \
	mnb-pasteboard-marshal.list

org.moblin.UX.Shell.Panels.pasteboard.service: org.moblin.UX.Shell.Panels.pasteboard.service.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

mnb-pasteboard-marshal.h: stamp-mnb-pasteboard-marshal.h
	@true
stamp-mnb-pasteboard-marshal.h: mnb-pasteboard-marshal.list
	$(QUIET_GEN)$(GLIB_GENMARSHAL) \
		--prefix=mnb_pasteboard_marshal \
		--header \
	$(srcdir)/mnb-pasteboard-marshal.list > xgen-mh \
	&& (cmp -s xgen-mh mnb-pasteboard-marshal.h || cp -f xgen-mh mnb-pasteboard-marshal.h ) \
	&& rm -f xgen-mh \
	&& echo timestamp > $(@F)

mnb-pasteboard-marshal.c: mnb-pasteboard-marshal.h
	$(QUIET_GEN)( echo "#include \"mnb-pasteboard-marshal.h\"" ; \
	$(GLIB_GENMARSHAL) \
		--prefix=mnb_pasteboard_marshal \
		--body \
	$(srcdir)/mnb-pasteboard-marshal.list ) > xgen-mc \
	&& cp -f xgen-mc mnb-pasteboard-marshal.c \
	&& rm -f xgen-mc


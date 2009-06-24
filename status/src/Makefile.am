QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

INCLUDES = \
	-I$(top_srcdir)/status \
	-I$(top_srcdir)/libmoblin-panel \
	-DG_LOG_DOMAIN=\"Mutter-Moblin-Status\" \
	-DPREFIX=\"$(prefix)\" \
	-DDATADIR=\"$(datadir)\" \
	-DPKGDATADIR=\"$(datadir)/mutter-moblin/status\" \
	-DMUTTER_MOBLIN_CSS=\"$(datadir)/mutter-moblin/theme/mutter-moblin.css\"

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

moblin_panel_status_CPPFLAGS = $(STATUS_CFLAGS)
moblin_panel_status_LDADD    = $(STATUS_LIBS) $(top_builddir)/libmoblin-panel/moblin-panel/libmoblin-panel-clutter.la
moblin_panel_status_DEPENDENCIES = $(top_builddir)/libmoblin-panel/moblin-panel/libmoblin-panel-clutter.la
moblin_panel_status_SOURCES  = 	\
	$(BUILT_SOURCES)	\
	mnb-im-status-row.h	\
	mnb-im-status-row.c	\
	mnb-web-status-entry.h	\
	mnb-web-status-entry.c	\
	mnb-web-status-row.h	\
	mnb-web-status-row.c	\
	moblin-panel-status.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.Mnb.Panels.status.service.in
service_DATA = org.moblin.Mnb.Panels.status.service

EXTRA_DIST = $(service_in_files) mnb-status-marshal.list

CLEANFILES = stamp-mnb-status-marshal.h

DISTCLEANFILES = org.moblin.Mnb.Panels.status.service $(BUILT_SOURCES)

org.moblin.Mnb.Panels.status.service: org.moblin.Mnb.Panels.status.service.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

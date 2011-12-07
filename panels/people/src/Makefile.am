QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PANEL_PEOPLE_CFLAGS) \
	-I$(top_srcdir)/panels/people/anerley \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	$(NULL)

libexec_PROGRAMS = dawati-panel-people

dawati_panel_people_LDADD = \
	$(PANEL_PEOPLE_LIBS) \
	$(top_builddir)/panels/people/anerley/anerley/libanerley.la \
	$(NULL)

%.c: %.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --body --prefix=sw_marshal $< > $@

%.h: %.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --header --prefix=sw_marshal $< > $@

BUILT_SOURCES = sw-marshals.c sw-marshals.h

dawati_panel_people_SOURCES = \
	mnb-people-panel.c \
	mnb-people-panel.h \
	dawati-panel-people.c \
	sw-online.c \
	sw-online.h \
	sw-marshals.list \
	$(BUILT_SOURCES) \
	$(NULL)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.people.service.in
service_DATA = com.dawati.UX.Shell.Panels.people.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = com.dawati.UX.Shell.Panels.people.service $(BUILT_SOURCES)

com.dawati.UX.Shell.Panels.people.service: com.dawati.UX.Shell.Panels.people.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PEOPLE_CFLAGS) \
	$(MPL_CFLAGS) \
	$(ONLINE_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\"

libexec_PROGRAMS = meego-panel-people

meego_panel_people_LDADD = $(PEOPLE_LIBS) $(MPL_LIBS) $(ONLINE_LIBS)

%.c: %.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --body --prefix=sw_marshal $< > $@

%.h: %.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --header --prefix=sw_marshal $< > $@

BUILT_SOURCES = sw-marshals.c sw-marshals.h

meego_panel_people_SOURCES = \
  mnb-people-panel.c \
  mnb-people-panel.h \
  meego-panel-people.c \
  sw-online.c \
  sw-online.h \
  sw-marshals.list \
  $(BUILT_SOURCES)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.meego.UX.Shell.Panels.people.service.in
service_DATA = com.meego.UX.Shell.Panels.people.service

EXTRA_DIST = $(service_in_files) 

DISTCLEANFILES = com.meego.UX.Shell.Panels.people.service $(BUILT_SOURCES)

com.meego.UX.Shell.Panels.people.service: com.meego.UX.Shell.Panels.people.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

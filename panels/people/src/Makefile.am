QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PEOPLE_CFLAGS) \
	$(MPL_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\"

libexec_PROGRAMS = moblin-panel-people

moblin_panel_people_LDADD = $(PEOPLE_LIBS) $(MPL_LIBS)

moblin_panel_people_SOURCES = \
  mnb-people-panel.c \
  mnb-people-panel.h \
  moblin-panel-people.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.people.service.in
service_DATA = org.moblin.UX.Shell.Panels.people.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.people.service

org.moblin.UX.Shell.Panels.people.service: org.moblin.UX.Shell.Panels.people.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

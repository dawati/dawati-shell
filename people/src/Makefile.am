QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	    $(PEOPLE_CFLAGS) \
	    -DDATADIR=\"$(datadir)\" \
	    -DMUTTER_MOBLIN_CSS=\"$(datadir)/mutter-moblin/theme/mutter-moblin.css\" \
	    $(top_builddir)/libmoblin-panel/moblin-panel/libmoblin-panel-clutter.la \
	    -I$(top_srcdir) \
	    -I$(top_srcdir)/libmoblin-panel



libexec_PROGRAMS = moblin-panel-people

moblin_panel_people_LDADD = $(PEOPLE_LIBS)

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

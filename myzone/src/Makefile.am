QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	    $(PENGE_CFLAGS) \
	    -DDATADIR=\"$(datadir)\" \
	    -DMUTTER_MOBLIN_CSS=\"$(datadir)/mutter-moblin/theme/mutter-moblin.css\" \
	    -I$(top_srcdir) \
	    -I$(top_srcdir)/libmoblin-panel \
	    -I$(top_srcdir)/penge

libexec_PROGRAMS = moblin-panel-myzone

moblin_panel_myzone_LDADD = $(PENGE_LIBS) \
			    $(top_builddir)/libmoblin-panel/moblin-panel/libmoblin-panel-clutter.la \
			    $(top_builddir)/penge/penge/libpenge.la

moblin_panel_myzone_SOURCES = \
  moblin-panel-myzone.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.myzone.service.in
service_DATA = org.moblin.UX.Shell.Panels.myzone.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.myzone.service

org.moblin.UX.Shell.Panels.myzone.service: org.moblin.UX.Shell.Panels.myzone.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

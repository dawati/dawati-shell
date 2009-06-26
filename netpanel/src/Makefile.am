QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	    $(NETPANEL_CFLAGS) \
	    -DDATADIR=\"$(datadir)\" \
	    -DMUTTER_MOBLIN_CSS=\"$(datadir)/mutter-moblin/theme/mutter-moblin.css\" \
	    -I$(top_srcdir) \
	    -I$(top_srcdir)/libmoblin-panel \
	    -I$(top_srcdir)/penge -I$(top_srcdir)/src



libexec_PROGRAMS = moblin-panel-internet

moblin_panel_internet_LDADD = $(NETPANEL_LIBS) \
			      $(top_builddir)/libmoblin-panel/moblin-panel/libmoblin-panel-clutter.la \
			      $(top_builddir)/penge/penge/libpenge.la


moblin_panel_internet_SOURCES = \
  moblin-panel-internet.c   \
  moblin-netbook-netpanel.c \
  moblin-netbook-netpanel.h \
  mnb-netpanel-bar.c        \
  mnb-netpanel-bar.h        \
  mwb-ac-list.c             \
  mwb-ac-list.h             \
  mwb-separator.c           \
  mwb-separator.h           \
  mwb-utils.c               \
  mwb-utils.h

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.internet.service.in
service_DATA = org.moblin.UX.Shell.Panels.internet.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.internet.service

org.moblin.UX.Shell.Panels.internet.service: org.moblin.UX.Shell.Panels.internet.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@


QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(APPLICATIONS_CFLAGS) \
	-DDATADIR=\"$(datadir)\" \
	-DMUTTER_MOBLIN_CSS=\"$(datadir)/mutter-moblin/theme/mutter-moblin.css\"

libexec_PROGRAMS = moblin-panel-applications

moblin_panel_applications_LDADD = $(APPLICATIONS_LIBS)

moblin_panel_applications_SOURCES = \
  mnb-launcher-application.c \
  mnb-launcher-application.h \
  mnb-launcher-button.c \
  mnb-launcher-button.h \
  mnb-launcher-grid.c \
  mnb-launcher-grid.h \
  mnb-launcher-tree.c \
  mnb-launcher-tree.h \
  moblin-netbook-launcher.c \
  moblin-netbook-launcher.h \
  moblin-panel-applications.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.Mnb.Panels.applications.service.in
service_DATA = org.moblin.Mnb.Panels.applications.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.Mnb.Panels.applications.service

org.moblin.Mnb.Panels.applications.service: org.moblin.Mnb.Panels.applications.service.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@


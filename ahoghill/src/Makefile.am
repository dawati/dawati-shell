
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(AHOGHILL_CFLAGS) \
	-I$(top_srcdir)/ahoghill \
	-DDATADIR=\"$(datadir)\" \
	-DPKGDATADIR=\"$(datadir)/moblin-panel-media\" \
	-I$(top_srcdir)/libmoblin-panel

# FIXME use this once split out
#	-DPKGDATADIR=\"$(pkgdatadir)\"

libexec_PROGRAMS = moblin-panel-media

moblin_panel_media_LDADD = $(AHOGHILL_LIBS)	\
	$(top_builddir)/libmoblin-panel/moblin-panel/libmoblin-panel-clutter.la \
	$(top_builddir)/ahoghill/ahoghill/libahoghill.la

moblin_panel_media_SOURCES = \
  moblin-panel-media.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.media.service.in
service_DATA = org.moblin.UX.Shell.Panels.media.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.media.service

org.moblin.UX.Shell.Panels.media.service: org.moblin.UX.Shell.Panels.media.service.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@


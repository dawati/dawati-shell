
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PASTEBOARD_CFLAGS) \
	-DDATADIR=\"$(datadir)\" \
	-DPKGDATADIR=\"$(datadir)/moblin-panel-pasteboard\" \
	-DMUTTER_MOBLIN_CSS=\"$(datadir)/mutter-moblin/theme/mutter-moblin.css\"

# FIXME use this once split out
#	-DPKGDATADIR=\"$(pkgdatadir)\"

libexec_PROGRAMS = moblin-panel-pasteboard

moblin_panel_pasteboard_LDADD = $(PASTEBOARD_LIBS)

moblin_panel_pasteboard_SOURCES = \
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

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.pasteboard.service

org.moblin.UX.Shell.Panels.pasteboard.service: org.moblin.UX.Shell.Panels.pasteboard.service.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@


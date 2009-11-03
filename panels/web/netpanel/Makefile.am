AM_CFLAGS = \
	$(SQLITE_CFLAGS) \
	$(NBTK_CFLAGS) \
	$(GTK_CFLAGS) \
	$(NETPANEL_CFLAGS) \
	$(MPL_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/netpanel\" \
	-I$(top_srcdir)/common

libexec_PROGRAMS = moblin-panel-web

moblin_panel_web_LDADD = \
	$(SQLITE_LIBS) \
	$(NBTK_LIBS) \
	$(GTK_LIBS) \
	$(NETPANEL_LIBS) \
	$(MPL_LIBS) \
	$(top_builddir)/common/libcommon.a

moblin_panel_web_SOURCES = \
  moblin-panel-web.c   \
  moblin-netbook-netpanel.c \
  moblin-netbook-netpanel.h \
  mnb-netpanel-bar.c        \
  mnb-netpanel-bar.h        \
  mnb-netpanel-scrollview.c \
  mnb-netpanel-scrollview.h

moblin_panel_web_DEPENDENCIES = \
  $(top_builddir)/common/libcommon.a

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.internet.service.in
service_DATA = org.moblin.UX.Shell.Panels.internet.service

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = moblin-panel-web.desktop

EXTRA_DIST = $(service_in_files) $(autostart_DATA)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.internet.service

org.moblin.UX.Shell.Panels.internet.service: org.moblin.UX.Shell.Panels.internet.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

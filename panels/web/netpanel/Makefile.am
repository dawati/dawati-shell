AM_CPPFLAGS = \
	$(SQLITE_CFLAGS) \
	$(MX_CFLAGS) \
	$(GTK_CFLAGS) \
	$(NETPANEL_CFLAGS) \
	$(MPL_CFLAGS) \
	$(CHROME_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DMX_CACHE=\"$(pkgdatadir)/mx.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/netpanel\" \
	-fno-exceptions -fno-rtti \
	-I$(top_srcdir)/common

libexec_PROGRAMS = moblin-panel-web

moblin_panel_web_LDADD = \
	$(SQLITE_LIBS) \
	$(MX_LIBS) \
	$(GTK_LIBS) \
	$(NETPANEL_LIBS) \
	$(MPL_LIBS) \
	$(CHROME_LIBS) \
	$(top_builddir)/common/libcommon.a

moblin_panel_web_SOURCES = \
  moblin-panel-web.cc   \
  moblin-netbook-netpanel.cc \
  moblin-netbook-netpanel.h \
  mnb-netpanel-bar.cc        \
  mnb-netpanel-bar.h        \
  mnb-netpanel-scrollview.cc \
  mnb-netpanel-scrollview.h 

moblin_panel_web_DEPENDENCIES = \
  $(top_builddir)/common/libcommon.a

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.internet.service.in
service_DATA = org.moblin.UX.Shell.Panels.internet.service

autostartdir = $(datadir)/mutter-moblin/panels
autostart_DATA = moblin-panel-internet.desktop

EXTRA_DIST = $(service_in_files) $(autostart_DATA)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.internet.service

org.moblin.UX.Shell.Panels.internet.service: org.moblin.UX.Shell.Panels.internet.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

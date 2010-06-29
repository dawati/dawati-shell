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

libexec_PROGRAMS = meego-panel-web

meego_panel_web_LDADD = \
	$(SQLITE_LIBS) \
	$(MX_LIBS) \
	$(GTK_LIBS) \
	$(NETPANEL_LIBS) \
	$(MPL_LIBS) \
	$(CHROME_LIBS) \
	$(top_builddir)/common/libcommon.a

meego_panel_web_SOURCES = \
  meego-panel-web.cc   \
  meego-netbook-netpanel.cc \
  meego-netbook-netpanel.h \
  mnb-netpanel-bar.cc        \
  mnb-netpanel-bar.h        \
  mnb-netpanel-scrollview.cc \
  mnb-netpanel-scrollview.h 

meego_panel_web_DEPENDENCIES = \
  $(top_builddir)/common/libcommon.a

servicedir = $(datadir)/dbus-1/services
service_in_files = org.meego.UX.Shell.Panels.internet.service.in
service_DATA = org.meego.UX.Shell.Panels.internet.service

autostartdir = $(datadir)/mutter-meego/panels
autostart_DATA = meego-panel-internet.desktop

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=meego-panel-internet.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

%.desktop.in: %.desktop.in.in Makefile
	$(AM_V_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = $(service_in_files) $(autostart_DATA)

DISTCLEANFILES = org.meego.UX.Shell.Panels.internet.service

org.meego.UX.Shell.Panels.internet.service: org.meego.UX.Shell.Panels.internet.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

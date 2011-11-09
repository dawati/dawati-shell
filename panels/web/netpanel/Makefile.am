AM_CPPFLAGS = \
	$(SQLITE_CFLAGS) \
	$(MX_CFLAGS) \
	$(GTK_CFLAGS) \
	$(NETPANEL_CFLAGS) \
	$(MPL_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DMX_CACHE=\"$(pkgdatadir)/mx.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/netpanel\" \
	-fno-exceptions -fno-rtti \
	-I$(top_srcdir)/common

libexec_PROGRAMS = dawati-panel-web

dawati_panel_web_LDADD = \
	$(SQLITE_LIBS) \
	$(MX_LIBS) \
	$(GTK_LIBS) \
	$(NETPANEL_LIBS) \
	$(MPL_LIBS) \
	$(top_builddir)/common/libcommon.a

dawati_panel_web_SOURCES = \
  dawati-panel-web.cc   \
  dawati-netbook-netpanel.cc \
  dawati-netbook-netpanel.h \
  mnb-netpanel-bar.cc        \
  mnb-netpanel-bar.h        \
  mnb-netpanel-scrollview.cc \
  mnb-netpanel-scrollview.h

dawati_panel_web_DEPENDENCIES = \
  $(top_builddir)/common/libcommon.a

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.internet.service.in
service_DATA = com.dawati.UX.Shell.Panels.internet.service

autostartdir = $(DAWATI_PANELS_DIR)
autostart_DATA = dawati-panel-internet.desktop

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=dawati-panel-internet.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

%.desktop.in: %.desktop.in.in Makefile
	$(AM_V_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = $(service_in_files) $(autostart_DATA)

DISTCLEANFILES = com.dawati.UX.Shell.Panels.internet.service

com.dawati.UX.Shell.Panels.internet.service: com.dawati.UX.Shell.Panels.internet.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

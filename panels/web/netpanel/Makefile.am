AM_CFLAGS = \
	$(MOZHELPER_CFLAGS) \
	$(NBTK_CFLAGS) \
	$(GTK_CFLAGS) \
	$(NETPANEL_CFLAGS) \
	$(MPL_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/netpanel\" \
	-I$(top_srcdir)/common

libexec_PROGRAMS = moblin-panel-internet

moblin_panel_internet_LDADD = \
	$(MOZHELPER_LIBS) \
	$(NBTK_LIBS) \
	$(GTK_LIBS) \
	$(NETPANEL_LIBS) \
	$(MPL_LIBS) \
	$(top_builddir)/common/libcommon.a

moblin_panel_internet_SOURCES = \
  moblin-panel-internet.c   \
  moblin-netbook-netpanel.c \
  moblin-netbook-netpanel.h \
  mnb-netpanel-bar.c        \
  mnb-netpanel-bar.h        \
  mnb-netpanel-scrollview.c \
  mnb-netpanel-scrollview.h

moblin_panel_internet_DEPENDENCIES = \
  $(top_builddir)/common/libcommon.a

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.internet.service.in
service_DATA = org.moblin.UX.Shell.Panels.internet.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.internet.service

org.moblin.UX.Shell.Panels.internet.service: org.moblin.UX.Shell.Panels.internet.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

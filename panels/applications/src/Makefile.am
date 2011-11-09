
AM_CFLAGS = \
	$(APPLICATIONS_CFLAGS) \
	$(MPL_CFLAGS) \
	-DDATADIR=\"$(datadir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DMX_CACHE=\"$(pkgdatadir)/mx.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\"

libexec_PROGRAMS = dawati-panel-applications

dawati_panel_applications_LDADD = $(APPLICATIONS_LIBS) $(MPL_LIBS)

dawati_panel_applications_SOURCES = \
  mnb-expander.c \
  mnb-expander.h \
  mnb-filter.c \
  mnb-filter.h \
  mnb-launcher-application.c \
  mnb-launcher-application.h \
  mnb-launcher-button.c \
  mnb-launcher-button.h \
  mnb-launcher-grid.c \
  mnb-launcher-grid.h \
  mnb-launcher-tree.c \
  mnb-launcher-tree.h \
  dawati-netbook-launcher.c \
  dawati-netbook-launcher.h \
  dawati-panel-applications.c

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.applications.service.in
service_DATA = com.dawati.UX.Shell.Panels.applications.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = com.dawati.UX.Shell.Panels.applications.service

com.dawati.UX.Shell.Panels.applications.service: com.dawati.UX.Shell.Panels.applications.service.in $(top_builddir)/config.log
	$(AM_V_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@


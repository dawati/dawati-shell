libexec_PROGRAMS = dawati-panel-applications

AM_CFLAGS = \
	$(LIBMPL_CFLAGS) \
	$(PANEL_APPLICATIONS_CFLAGS) \
	\
	-DMX_CACHE=\"$(pkgdatadir)/mx.cache\" \
	-DSTYLEDIR=\"$(DAWATI_STYLE_DIR)\" \
	$(NULL)

dawati_panel_applications_LDADD = \
	$(LIBMPL_LIBS) \
	$(PANEL_APPLICATIONS_LIBS) \
	$(NULL)

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

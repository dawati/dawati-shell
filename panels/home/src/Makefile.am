QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PANEL_HOME_CFLAGS) \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	-DPLUGINS_DIR=\"$(datadir)/dawati-shell/plugins/\" \
	$(NULL)

libexec_PROGRAMS = \
	dawati-panel-home \
	$(NULL)

noinst_PROGRAMS = \
	dawati-plugin-launcher \
	$(NULL)

noinst_LTLIBRARIES = \
	libmnb-home.la \
	libdawati-home-plugins.la \
	$(NULL)

DAWATI_HOME_LIBS = \
	$(PANEL_HOME_LIBS) \
	libmnb-home.la \
	libdawati-home-plugins.la \
	$(NULL)

dawati_panel_home_LDADD = $(DAWATI_HOME_LIBS)

dawati_panel_home_SOURCES = \
	dawati-panel-home.c \
	mnb-home-grid.c \
	mnb-home-grid.h \
	mnb-home-grid-child.c \
	mnb-home-grid-child.h \
	mnb-home-grid-private.h \
	mnb-home-new-widget-dialog.c \
	mnb-home-new-widget-dialog.h \
	mnb-home-panel.c \
	mnb-home-panel.h \
	mnb-home-widget.c \
	mnb-home-widget.h \
	mnb-home-widget-preview.c \
	mnb-home-widget-preview.h \
	$(NULL)

dawati_plugin_launcher_LDADD = $(DAWATI_HOME_LIBS)

dawati_plugin_launcher_SOURCES = \
	dawati-plugin-launcher.c \
	$(NULL)

libmnb_home_la_SOURCES = \
	mnb-home-plugins-engine.c \
	mnb-home-plugins-engine.h \
	utils.c \
	utils.h \
	$(NULL)

libdawati_home_plugins_la_SOURCES = \
	dawati-home-plugins-app.c \
	dawati-home-plugins-app.h \
	$(NULL)

include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = DawatiHomePlugins-1.0.gir

DawatiHomePlugins-1.0.gir: libdawati-home-plugins.la Makefile.am
DawatiHomePlugins_1_0_gir_NAMESPACE = DawatiHomePlugins
DawatiHomePlugins_1_0_gir_VERSION = 1.0
DawatiHomePlugins_1_0_gir_LIBS = libdawati-home-plugins.la
DawatiHomePlugins_1_0_gir_FILES = $(libdawati_home_plugins_la_SOURCES)
DawatiHomePlugins_1_0_gir_INCLUDES = Clutter-1.0

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

DISTCLEANFILES = \
	$(BUILT_SOURCES) \
	$(NULL)

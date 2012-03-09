QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PANEL_HOME_CFLAGS) \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	$(NULL)

libexec_PROGRAMS = dawati-panel-home

dawati_panel_home_LDADD = \
	$(PANEL_HOME_LIBS) \
	$(NULL)

# %.c: %.list Makefile.am
# 	$(QUIET_GEN)glib-genmarshal --body --prefix=sw_marshal $< > $@
#
# %.h: %.list Makefile.am
# 	$(QUIET_GEN)glib-genmarshal --header --prefix=sw_marshal $< > $@

# BUILT_SOURCES = sw-marshals.c sw-marshals.h

dawati_panel_home_SOURCES = \
	mnb-home-panel.c \
	mnb-home-panel.h \
	mnb-home-widget.c \
	mnb-home-widget.h \
	dawati-panel-home.c \
	$(BUILT_SOURCES) \
	$(NULL)

DISTCLEANFILES = \
	$(BUILT_SOURCES) \
	$(NULL)

QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	    $(PENGE_CFLAGS) \
	    -I$(top_srcdir)/libmoblin-panel \
	    -I$(top_srcdir)/penge \
	    -DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/moblin-panel-myzone\"

# FIXME use this once split out
# -DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)\"

libexec_PROGRAMS = moblin-panel-myzone

moblin_panel_myzone_LDADD = $(PENGE_LIBS) \
			    $(top_builddir)/libmoblin-panel/moblin-panel/libmoblin-panel-clutter.la \
			    $(top_builddir)/penge/penge/libpenge.la

moblin_panel_myzone_SOURCES = \
  moblin-panel-myzone.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.myzone.service.in
service_DATA = org.moblin.UX.Shell.Panels.myzone.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.myzone.service

org.moblin.UX.Shell.Panels.myzone.service: org.moblin.UX.Shell.Panels.myzone.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

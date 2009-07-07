QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(NETPANEL_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache\" \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/moblin-panel-internet\"

# FIXME use this once split out
# -DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)\"

libexec_PROGRAMS = moblin-panel-internet

moblin_panel_internet_LDADD = \
	$(NETPANEL_LIBS)

moblin_panel_internet_SOURCES = \
  moblin-panel-internet.c   \
  moblin-netbook-netpanel.c \
  moblin-netbook-netpanel.h \
  mnb-netpanel-bar.c        \
  mnb-netpanel-bar.h        \
  mwb-ac-list.c             \
  mwb-ac-list.h             \
  mwb-separator.c           \
  mwb-separator.h           \
  mwb-utils.c               \
  mwb-utils.h

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.internet.service.in
service_DATA = org.moblin.UX.Shell.Panels.internet.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.internet.service

org.moblin.UX.Shell.Panels.internet.service: org.moblin.UX.Shell.Panels.internet.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

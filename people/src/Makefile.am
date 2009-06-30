QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PEOPLE_CFLAGS) \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/moblin-panel-people\"

# FIXME use this once split out
# -DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)\"

libexec_PROGRAMS = moblin-panel-people

moblin_panel_people_LDADD = $(PEOPLE_LIBS)

moblin_panel_people_SOURCES = \
  mnb-people-panel.c \
  mnb-people-panel.h \
  moblin-panel-people.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.people.service.in
service_DATA = org.moblin.UX.Shell.Panels.people.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.people.service

org.moblin.UX.Shell.Panels.people.service: org.moblin.UX.Shell.Panels.people.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

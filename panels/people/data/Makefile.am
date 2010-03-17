SUBDIRS=theme

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=moblin-panel-people.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MOBLIN_DESKTOP_FILE
desktopfilesdir=$(MOBLIN_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)
endif

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

servicefiledir = $(datadir)/dbus-1/services
servicefile_in_files = org.freedesktop.Telepathy.Client.MoblinPanelPeople.service.in
servicefile_DATA = $(servicefile_in_files:.service.in=.service)

org.freedesktop.Telepathy.Client.MoblinPanelPeople.service: org.freedesktop.Telepathy.Client.MoblinPanelPeople.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

clientfiledir = $(datarootdir)/telepathy/clients
clientfile_DATA = MoblinPanelPeople.client

EXTRA_DIST = moblin-panel-people.desktop.in.in \
	     $(clientfile_DATA) \
	     $(servicefile_in_files)
CLEANFILES = moblin-panel-people.desktop moblin-panel-people.desktop.in $(servicefile_DATA)

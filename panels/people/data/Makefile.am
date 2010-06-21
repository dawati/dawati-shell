SUBDIRS=theme

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=meego-panel-people.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MEEGO_DESKTOP_FILE
desktopfilesdir=$(MEEGO_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)
endif

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

servicefiledir = $(datadir)/dbus-1/services
servicefile_in_files = org.freedesktop.Telepathy.Client.MeegoPanelPeople.service.in
servicefile_DATA = $(servicefile_in_files:.service.in=.service)

org.freedesktop.Telepathy.Client.MeegoPanelPeople.service: org.freedesktop.Telepathy.Client.MeegoPanelPeople.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

clientfiledir = $(datarootdir)/telepathy/clients
clientfile_DATA = MeegoPanelPeople.client

EXTRA_DIST = meego-panel-people.desktop.in.in \
	     $(clientfile_DATA) \
	     $(servicefile_in_files)
CLEANFILES = meego-panel-people.desktop meego-panel-people.desktop.in $(servicefile_DATA)

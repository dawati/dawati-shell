@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files = dawati-panel-people.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

desktopfilesdir = $(DAWATI_PANELS_DIR)
desktopfiles_DATA = $(desktopfiles_files)

servicefiledir = $(datadir)/dbus-1/services
servicefile_in_files = org.freedesktop.Telepathy.Client.DawatiPanelPeople.service.in
servicefile_DATA = $(servicefile_in_files:.service.in=.service)

org.freedesktop.Telepathy.Client.DawatiPanelPeople.service: org.freedesktop.Telepathy.Client.DawatiPanelPeople.service.in
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

clientfiledir = $(datarootdir)/telepathy/clients
clientfile_DATA = DawatiPanelPeople.client

EXTRA_DIST = \
	dawati-panel-people.desktop.in.in \
	$(servicefile_in_files) \
	$(clientfile_DATA) \
	$(NULL)
CLEANFILES = \
	$(desktopfiles_in_files) \
	$(desktopfiles_files) \
	$(servicefile_DATA) \
	$(NULL)

include $(top_srcdir)/build/autotools/Makefile.am.panels

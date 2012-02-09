
@INTLTOOL_DESKTOP_RULE@

desktopfilesdir = $(datadir)/applications
desktopfiles_in_files = \
	dawati-toolbar-properties.desktop.in \
	system-tray-properties.desktop.in \
	$(NULL)

#	dawati-applications-panel-properties.desktop.in

desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)
desktopfiles_DATA = $(desktopfiles_files)

themedir = $(DAWATI_RUNTIME_THEME_DIR)
dist_theme_DATA = \
	theme/dawati-toolbar-properties.css

CLEANFILES = $(desktopfiles_DATA)
EXTRA_DIST = $(desktopfiles_in_files)

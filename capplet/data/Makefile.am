
@INTLTOOL_DESKTOP_RULE@

desktopfilesdir = $(datadir)/applications
desktopfiles_in_files = meego-toolbar-properties.desktop.in	\
			system-tray-properties.desktop.in

desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)
desktopfiles_DATA = $(desktopfiles_files)

themedir = $(MUTTER_MEEGO_THEME_DIR)
dist_theme_DATA = \
	theme/meego-toolbar-properties.css

CLEANFILES = $(desktopfiles_DATA)
EXTRA_DIST = $(desktopfiles_in_files)

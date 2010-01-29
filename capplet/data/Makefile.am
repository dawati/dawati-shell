
@INTLTOOL_DESKTOP_RULE@

desktopfilesdir = $(datadir)/applications
desktopfiles_in_files = moblin-toolbar-properties.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)
desktopfiles_DATA = $(desktopfiles_files)

themedir = $(MUTTER_MOBLIN_THEME_DIR)
dist_theme_DATA = \
	theme/moblin-toolbar-properties.css

CLEANFILES = $(desktopfiles_DATA)
EXTRA_DIST = $(desktopfiles_in_files)

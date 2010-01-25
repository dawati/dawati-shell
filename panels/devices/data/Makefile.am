
SUBDIRS = theme

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=moblin-panel-devices.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MOBLIN_DESKTOP_FILE
desktopfilesdir=$(MOBLIN_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)
endif

%.desktop.in: %.desktop.in.in Makefile
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = moblin-panel-devices.desktop.in.in
CLEANFILES = moblin-panel-devices.desktop moblin-panel-devices.desktop.in

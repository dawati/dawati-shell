SUBDIRS=style

desktopfiles_in_files=moblin-panel-zones.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MOBLIN_DESKTOP_FILE
desktopfilesdir=$(MOBLIN_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)
endif

%.desktop: %.desktop.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = moblin-panel-zones.desktop.in
CLEANFILES = moblin-panel-zones.desktop moblin-panel-zones.desktop
CLEANFILES = moblin-panel-zones.desktop

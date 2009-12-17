SUBDIRS=theme

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=moblin-panel-status.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MOBLIN_DESKTOP_FILE
desktopfilesdir=$(MOBLIN_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)
endif

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = moblin-panel-status.desktop.in.in
CLEANFILES = moblin-panel-status.desktop moblin-panel-status.desktop.in
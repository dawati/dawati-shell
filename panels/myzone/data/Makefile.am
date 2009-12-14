SUBDIRS=theme

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=moblin-panel-myzone.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MOBLIN_DESKTOP_FILE
desktopfilesdir=$(MOBLIN_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)
endif

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

# would prefer to just symlink this, but could make that pass distcheck
autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktopfiles_files)

EXTRA_DIST = moblin-panel-myzone.desktop.in.in
CLEANFILES = moblin-panel-myzone.desktop moblin-panel-myzone.desktop.in
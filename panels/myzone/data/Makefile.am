SUBDIRS=theme

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=moblin-panel-myzone.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MOBLIN_DESKTOP_FILE
panelfilesdir=$(MOBLIN_PANELS_DIR)/
panelfiles_DATA = $(desktopfiles_files)
endif

settingsfiles_in_files=myzone.desktop.in
settingsfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)
settingsdir = $(datadir)/applications
settings_DATA = $(settingsfiles_files)

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

# would prefer to just symlink this, but could make that pass distcheck
autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktopfiles_files)

uidir = $(pkgdatadir)
ui_DATA = capplet.ui

EXTRA_DIST = moblin-panel-myzone.desktop.in.in myzone.in.in
CLEANFILES = moblin-panel-myzone.desktop moblin-panel-myzone.desktop.in \
	     myzone.desktop myzone.desktop.in

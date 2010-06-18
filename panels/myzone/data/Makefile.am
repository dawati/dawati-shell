SUBDIRS=theme

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=meego-panel-myzone.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MEEGO_DESKTOP_FILE
panelfilesdir=$(MEEGO_PANELS_DIR)/
panelfiles_DATA = $(desktopfiles_files)
endif

settingsfiles_in_files=myzone.desktop.in
settingsfiles_files=$(settingsfiles_in_files:.desktop.in=.desktop)
settingsdir = $(datadir)/applications
settings_DATA = $(settingsfiles_files)

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

# would prefer to just symlink this, but could make that pass distcheck
autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktopfiles_files)

if ENABLE_CAPPLET
uidir = $(pkgdatadir)
ui_DATA = capplet.ui
endif

schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = meego-panel-myzone.schemas

install-data-local:
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool-2 --makefile-install-rule $(schema_DATA) 2>&1 > /dev/null; \
	fi
endif

EXTRA_DIST = meego-panel-myzone.desktop.in.in myzone.desktop.in.in meego-panel-myzone.schemas.in capplet.ui
CLEANFILES = meego-panel-myzone.desktop meego-panel-myzone.desktop.in \
	     myzone.desktop myzone.desktop.in

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files = dawati-panel-myzone.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

panelfilesdir = $(DAWATI_PANELS_DIR)
panelfiles_DATA = $(desktopfiles_files)

settingsfiles_in_files = myzone.desktop.in
settingsfiles_files = $(settingsfiles_in_files:.desktop.in=.desktop)

settingsdir = $(datadir)/applications
settings_DATA = $(settingsfiles_files)

# would prefer to just symlink this, but could make that pass distcheck
autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktopfiles_files)

if ENABLE_CAPPLET
uidir = $(pkgdatadir)
ui_DATA = capplet.ui
endif

schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = dawati-panel-myzone.schemas

install-data-local:
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool-2 --makefile-install-rule $(schema_DATA) 2>&1 > /dev/null; \
	fi
endif

EXTRA_DIST = \
	dawati-panel-myzone.desktop.in.in \
	myzone.desktop.in.in \
	dawati-panel-myzone.schemas.in \
	capplet.ui \
	$(NULL)
CLEANFILES = \
	$(desktopfiles_in_files) \
	$(desktopfiles_files) \
	$(schema_DATA)
	$(NULL)

include $(top_srcdir)/build/autotools/Makefile.am.panels

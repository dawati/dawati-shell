@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files = dawati-panel-switcher.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

desktopfilesdir = $(DAWATI_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktopfiles_files)

jsondir = $(PANEL_SWITCHER_DATADIR)
dist_json_DATA = switcher.json

EXTRA_DIST = dawati-panel-switcher.desktop.in.in
CLEANFILES = \
	$(desktopfiles_files) \
	$(desktopfiles_in_files) \
	$(NULL)

include $(top_srcdir)/build/autotools/Makefile.am.panels

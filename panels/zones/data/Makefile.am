SUBDIRS = theme

assetdir = $(PANEL_ZONES_DATADIR)
dist_asset_DATA = people.png

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files = dawati-panel-zones.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

desktopfilesdir = $(DAWATI_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)

EXTRA_DIST = dawati-panel-zones.desktop.in
CLEANFILES = dawati-panel-zones.desktop dawati-panel-zones.desktop.in

include $(top_srcdir)/build/autotools/Makefile.am.panels

-include $(top_srcdir)/git.mk

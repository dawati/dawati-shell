SUBDIRS=style

assetdir = $(pkgdatadir)
dist_asset_DATA = people.png

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=moblin-panel-zones.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MOBLIN_DESKTOP_FILE
desktopfilesdir=$(MOBLIN_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)
endif

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = moblin-panel-zones.desktop.in.in
CLEANFILES = moblin-panel-zones.desktop moblin-panel-zones.desktop.in

-include $(top_srcdir)/git.mk

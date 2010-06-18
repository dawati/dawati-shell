SUBDIRS=style

assetdir = $(pkgdatadir)
dist_asset_DATA = people.png

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=meego-panel-zones.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MEEGO_DESKTOP_FILE
desktopfilesdir=$(MEEGO_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)
endif

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = meego-panel-zones.desktop.in.in
CLEANFILES = meego-panel-zones.desktop meego-panel-zones.desktop.in

-include $(top_srcdir)/git.mk

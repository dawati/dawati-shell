SUBDIRS=theme

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files=meego-panel-status.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_MEEGO_DESKTOP_FILE
desktopfilesdir=$(MEEGO_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)
endif

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = meego-panel-status.desktop.in.in
CLEANFILES = meego-panel-status.desktop meego-panel-status.desktop.in

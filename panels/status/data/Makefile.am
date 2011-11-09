SUBDIRS = theme

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files = dawati-panel-status.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

if ENABLE_DAWATI_DESKTOP_FILE
desktopfilesdir = $(DAWATI_PANELS_DIR)
desktopfiles_DATA = $(desktopfiles_files)
endif

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = dawati-panel-status.desktop.in.in
CLEANFILES = dawati-panel-status.desktop dawati-panel-status.desktop.in

SUBDIRS = theme

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files = dawati-panel-status.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

desktopfilesdir = $(DAWATI_PANELS_DIR)
desktopfiles_DATA = $(desktopfiles_files)

EXTRA_DIST = dawati-panel-status.desktop.in
CLEANFILES = dawati-panel-status.desktop dawati-panel-status.desktop.in

include $(top_srcdir)/build/autotools/Makefile.am.panels

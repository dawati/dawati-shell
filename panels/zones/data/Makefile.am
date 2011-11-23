@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files = dawati-panel-zones.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

desktopfilesdir = $(DAWATI_PANELS_DIR)/
desktopfiles_DATA = $(desktopfiles_files)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktopfiles_files)

EXTRA_DIST = dawati-panel-zones.desktop.in.in
CLEANFILES = \
	$(desktopfiles_files) \
	$(desktopfiles_in_files) \
	$(NULL)

include $(top_srcdir)/build/autotools/Makefile.am.panels

-include $(top_srcdir)/git.mk

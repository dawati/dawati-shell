desktopfiles_in_files = dawati-panel-bluetooth.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

paneldir = $(DAWATI_PANELS_DIR)
panel_DATA = dawati-panel-bluetooth.desktop

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = dawati-panel-bluetooth.desktop

EXTRA_DIST = \
	dawati-panel-bluetooth.desktop.in.in \
	$(NULL)
CLEANFILES = \
	$(desktopfiles_in_files) \
	$(desktopfiles_files) \
	$(NULL)

include $(top_srcdir)/build/autotools/Makefile.am.panels

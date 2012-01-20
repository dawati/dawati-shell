desktopfiles_in_files = dawati-panel-bluetooth.desktop.in dawati-panel-bluetooth-standalone.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

paneldir = $(DAWATI_PANELS_DIR)
panel_DATA = dawati-panel-bluetooth.desktop

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = dawati-panel-bluetooth.desktop

if ENABLE_DAWATI_DESKTOP_FILE
desktopdir = $(datadir)/applications
desktop_DATA = dawati-panel-bluetooth-standalone.desktop
endif

EXTRA_DIST = \
	dawati-panel-bluetooth.desktop.in.in \
	dawati-panel-bluetooth-standalone.desktop.in.in \
	$(NULL)
CLEANFILES = \
	$(desktopfiles_in_files) \
	$(desktopfiles_files) \
	$(NULL)

include $(top_srcdir)/build/autotools/Makefile.am.panels

-include $(top_srcdir)/git.mk

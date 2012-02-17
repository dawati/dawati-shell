desktopfiles_in_files = carrick.desktop.in carrick-standalone.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

paneldir = $(DAWATI_PANELS_DIR)
panel_DATA = carrick.desktop

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = carrick.desktop

if ENABLE_DAWATI_DESKTOP_FILE
desktopdir = $(datadir)/applications
desktop_DATA = carrick-standalone.desktop
endif

EXTRA_DIST = \
	carrick.desktop.in.in \
	carrick-standalone.desktop.in.in \
	$(NULL)
CLEANFILES = \
	$(desktopfiles_files) \
	$(desktopfiles_in_files) \
	$(NULL)

include $(top_srcdir)/build/autotools/Makefile.am.panels

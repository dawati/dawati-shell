SUBDIRS = icons theme

desktop_in_files = carrick.desktop.in.in carrick-standalone.desktop.in.in
desktop_out_files = carrick.desktop carrick-standalone.desktop

@INTLTOOL_DESKTOP_RULE@

if ENABLE_DAWATI_DESKTOP_FILE
paneldir = $(DAWATI_PANELS_DIR)
panel_DATA = carrick.desktop

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = carrick.desktop
endif

desktopdir = $(datadir)/applications
desktop_DATA = carrick-standalone.desktop

EXTRA_DIST = $(panel_in_files)
CLEANFILES = $(panel_DATA)

-include $(top_srcdir)/git.mk

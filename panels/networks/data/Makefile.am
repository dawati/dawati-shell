SUBDIRS = icons theme

paneldir = @PANEL_DIR@
panel_in_files = carrick.desktop.in.in
panel_DATA = $(panel_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(panel_DATA)

EXTRA_DIST = $(panel_in_files)
CLEANFILES = $(panel_DATA)

-include $(top_srcdir)/git.mk

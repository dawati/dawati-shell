SUBDIRS = icons theme

desktop_in_files = carrick.desktop.in.in
desktop_data = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

if WITH_MEEGO
paneldir = @PANEL_DIR@
panel_DATA = $(desktop_data)
endif

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktop_data)

EXTRA_DIST = $(panel_in_files)
CLEANFILES = $(panel_DATA)

-include $(top_srcdir)/git.mk

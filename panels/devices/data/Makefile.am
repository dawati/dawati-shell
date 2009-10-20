SUBDIRS = icons theme

autostart_in_files = 	dalston-power-applet.desktop.in \
			dalston-volume-applet.desktop.in
autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

desktop_in_files = dalston-power-settings-capplet.desktop.in
if WITH_SETTINGS_CAPPLET
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
endif
@INTLTOOL_DESKTOP_RULE@

CLEANFILES = $(desktop_DATA)
EXTRA_DIST = $(autostart_DATA) $(desktop_in_files)

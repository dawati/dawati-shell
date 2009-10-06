SUBDIRS = icons theme

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = dalston-volume-applet.desktop dalston-power-applet.desktop

desktop_in_files = dalston-power-settings-capplet.desktop.in
if WITH_SETTINGS_CAPPLET
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
endif
@INTLTOOL_DESKTOP_RULE@

CLEANFILES = $(desktop_DATA)
EXTRA_DIST = $(autostart_DATA) $(desktop_in_files)

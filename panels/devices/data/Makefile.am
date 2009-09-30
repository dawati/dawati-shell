SUBDIRS = icons theme

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = dalston-volume-applet.desktop dalston-power-applet.desktop

desktopdir = $(datadir)/applications
desktop_in_files = dalston-power-settings-capplet.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

CLEANFILES = $(desktop_DATA)
EXTRA_DIST = $(autostart_DATA) $(desktop_in_files)

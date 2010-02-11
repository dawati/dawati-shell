
autostart_src_files = moblin-power-icon.desktop.in.in
autostart_in_files = moblin-power-icon.desktop.in
#autostartdir = $(sysconfdir)/xdg/autostart
autostartdir = $(MOBLIN_PANELS_DIR)
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

service_in_files = org.moblin.UX.Shell.Panels.power.service.in
servicedir = $(datadir)/dbus-1/services
service_DATA = org.moblin.UX.Shell.Panels.power.service

EXTRA_DIST = \
	$(autostart_src_files) \
	$(service_in_files) \
	$(NULL)

DISTCLEANFILES = \
	$(autostart_in_files) \
	$(autostart_DATA) \
	$(service_DATA) \
	$(NULL)

@INTLTOOL_DESKTOP_RULE@


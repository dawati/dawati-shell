
autostart_src_files = moblin-power-icon.desktop.in.in
autostart_in_files = moblin-power-icon.desktop.in
autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.power.service.in
service_DATA = org.moblin.UX.Shell.Panels.power.service

EXTRA_DIST = \
	$(autostart_src_files) \
	$(service_in_files) \
	$(NULL)

DISTCLEANFILES = \
	$(autostart_DATA) \
	$(service_DATA) \
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

%.service: %.service.in
	$(AM_V_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@


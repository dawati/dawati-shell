desktop_in_files = dawati-power-icon.desktop.in
desktop_data = $(desktop_in_files:.desktop.in=.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktop_data)

paneldir = $(DAWATI_PANELS_DIR)
panel_DATA = $(desktop_data)

service_in_files = com.dawati.UX.Shell.Panels.power.service.in
servicedir = $(datadir)/dbus-1/services
service_DATA = com.dawati.UX.Shell.Panels.power.service

EXTRA_DIST = \
	dawati-power-icon.desktop.in.in \
	$(service_in_files) \
	$(NULL)

CLEANFILES = \
	$(desktop_in_files) \
	$(panel_DATA) \
	$(service_DATA) \
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

include $(top_srcdir)/build/autotools/Makefile.am.panels

-include $(top_srcdir)/git.mk

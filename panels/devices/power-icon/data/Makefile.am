
SUBDIRS = theme

desktop_in_files = moblin-power-icon.desktop.in.in
desktop_data = $(desktop_in_files:.desktop.in.in=.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktop_data)

paneldir = $(MOBLIN_PANELS_DIR)
panel_DATA = $(desktop_data)

service_in_files = org.moblin.UX.Shell.Panels.power.service.in
servicedir = $(datadir)/dbus-1/services
service_DATA = org.moblin.UX.Shell.Panels.power.service

EXTRA_DIST = \
  $(desktop_in_files) \
  $(service_in_files) \
  $(NULL)

DISTCLEANFILES = \
  $(panel_DATA) \
  $(service_DATA) \
  $(NULL)

@INTLTOOL_DESKTOP_RULE@


-include $(top_srcdir)/git.mk

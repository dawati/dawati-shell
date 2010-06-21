
SUBDIRS = icons theme

autostart_src_files = meego-panel-devices.desktop.in.in
autostart_in_files = meego-panel-devices.desktop.in
#autostartdir = $(sysconfdir)/xdg/autostart
autostartdir = $(MEEGO_PANELS_DIR)
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

service_in_files = com.meego.UX.Shell.Panels.devices.service.in
servicedir = $(datadir)/dbus-1/services
service_DATA = com.meego.UX.Shell.Panels.devices.service

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


-include $(top_srcdir)/git.mk

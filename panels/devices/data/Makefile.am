
SUBDIRS = icons theme

autostart_src_files = meego-panel-devices.desktop.in.in
autostart_in_files = meego-panel-devices.desktop.in
autostartdir = $(MEEGO_PANELS_DIR)
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

schema_in_files = meego-panel-devices.schemas.in
schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = meego-panel-devices.schemas

service_in_files = com.meego.UX.Shell.Panels.devices.service.in
servicedir = $(datadir)/dbus-1/services
service_DATA = com.meego.UX.Shell.Panels.devices.service

EXTRA_DIST = \
  $(autostart_src_files) \
  $(schema_in_files) \
  $(service_in_files) \
  $(NULL)

DISTCLEANFILES = \
  $(autostart_in_files) \
  $(autostart_DATA) \
  $(schema_DATA) \
  $(service_DATA) \
  $(NULL)

@INTLTOOL_DESKTOP_RULE@

install-data-local:
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) gconftool-2 --makefile-install-rule $(schema_DATA) 2>&1 > /dev/null; \
	fi
endif

-include $(top_srcdir)/git.mk

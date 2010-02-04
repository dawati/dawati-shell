
AM_CFLAGS = \
	$(MPD_CFLAGS) \
	-I$(top_srcdir) \
	-DDATADIR=\"$(datadir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DMX_CACHE=\"$(pkgdatadir)/mx.cache\" \
	-DICONDIR=\"$(pkgdatadir)/icons\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\" \
	-DGDU_API_IS_SUBJECT_TO_CHANGE \
	-DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
	$(NULL)

libexec_PROGRAMS = moblin-panel-devices

moblin_panel_devices_LDADD = \
	$(top_builddir)/egg-console-kit/libegg-console-kit.la \
	$(top_builddir)/gdu/libgdu.la \
	$(MPD_LIBS) \
  $(NULL)

moblin_panel_devices_SOURCES = \
	moblin-panel-devices.c \
	mpd-battery-device.c \
	mpd-battery-device.h \
	mpd-battery-tile.c \
	mpd-battery-tile.h \
	mpd-computer-pane.c \
	mpd-computer-pane.h \
	mpd-disk-device.c \
	mpd-disk-device.h \
	mpd-disk-tile.c \
	mpd-disk-tile.h \
	mpd-global-key.c \
	mpd-global-key.h \
	mpd-folder-button.c \
	mpd-folder-button.h \
	mpd-folder-pane.c \
	mpd-folder-pane.h \
	mpd-folder-store.c \
	mpd-folder-store.h \
	mpd-folder-view.c \
	mpd-folder-view.h \
	mpd-shell.c \
	mpd-shell.h \
	mpd-shell-defines.h \
	mpd-shutdown-notification.c \
	mpd-shutdown-notification.h \
	$(NULL)

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.devices.service.in
service_DATA = org.moblin.UX.Shell.Panels.devices.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.devices.service

org.moblin.UX.Shell.Panels.devices.service: org.moblin.UX.Shell.Panels.devices.service.in $(top_builddir)/config.log
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@


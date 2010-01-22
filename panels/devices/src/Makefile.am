
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(DEVICES_CFLAGS) \
	-I$(top_srcdir) \
	-DDATADIR=\"$(datadir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DMX_CACHE=\"$(pkgdatadir)/mx.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\"

libexec_PROGRAMS = moblin-panel-devices

moblin_panel_devices_LDADD = \
	../dalston/libdalston.la \
	$(DEVICES_LIBS) \
	$(LIBNOTIFY_LIBS)

moblin_panel_devices_SOURCES = \
	moblin-panel-devices.c \
	mpd-battery-tile.c \
	mpd-battery-tile.h \
	mpd-computer-pane.c \
	mpd-computer-pane.h \
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
	mpd-shutdown-notification.c \
	mpd-shutdown-notification.h \
	$(NULL)

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.devices.service.in
service_DATA = org.moblin.UX.Shell.Panels.devices.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.devices.service

org.moblin.UX.Shell.Panels.devices.service: org.moblin.UX.Shell.Panels.devices.service.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@



AM_CFLAGS = \
	$(MPD_CFLAGS) \
	-I$(top_srcdir) \
	-DDATADIR=\"$(datadir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DMX_CACHE=\"$(pkgdatadir)/mx.cache\" \
	-DICONDIR=\"$(pkgdatadir)/icons\" \
	-DPKGTHEMEDIR=\"$(PKGTHEMEDIR)\" \
	-DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
	$(NULL)

libexec_PROGRAMS = moblin-panel-devices

moblin_panel_devices_LDADD = \
	$(top_builddir)/gvc/libgvc.la \
	$(MPD_LIBS) \
  $(NULL)

if HAVE_UDISKS

AM_CFLAGS += -DGDU_API_IS_SUBJECT_TO_CHANGE
moblin_panel_devices_LDADD += $(top_builddir)/gdu/libgdu.la

endif

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
	mpd-volume-tile.c \
	mpd-volume-tile.h \
	$(NULL)


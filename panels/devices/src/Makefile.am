
AM_CFLAGS = \
  $(MPD_CFLAGS) \
  -I$(top_srcdir) \
  -DLOCALEDIR=\"$(localedir)\" \
  -DPKGDATADIR=\"$(pkgdatadir)\" \
  -DPKGICONDIR=\"$(pkgdatadir)/icons\" \
  -DPKGTHEMEDIR=\"$(PKGTHEMEDIR)\" \
  -DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
  $(NULL)

libexec_PROGRAMS = \
  moblin-panel-devices \
  $(NULL)

moblin_panel_devices_LDADD = \
  $(top_builddir)/gpm/libgpm.la \
  $(top_builddir)/gvc/libgvc.la \
  $(MPD_LIBS) \
  $(NULL)

moblin_panel_devices_SOURCES = \
  moblin-panel-devices.c \
  mpd-battery-device.c \
  mpd-battery-device.h \
  mpd-battery-icon.c \
  mpd-battery-icon.h \
  mpd-battery-tile.c \
  mpd-battery-tile.h \
  mpd-brightness-tile.c \
  mpd-brightness-tile.h \
  mpd-computer-pane.c \
  mpd-computer-pane.h \
  mpd-computer-tile.c \
  mpd-computer-tile.h \
  mpd-default-device-tile.c \
  mpd-default-device-tile.h \
  mpd-devices-pane.c \
  mpd-devices-pane.h \
  mpd-devices-tile.c \
  mpd-devices-tile.h \
  mpd-display-device.c \
  mpd-display-device.h \
  mpd-disk-tile.c \
  mpd-disk-tile.h \
  mpd-folder-button.c \
  mpd-folder-button.h \
  mpd-folder-tile.c \
  mpd-folder-tile.h \
  mpd-gobject.c \
  mpd-gobject.h \
  mpd-panel.c \
  mpd-panel.h \
  mpd-shell.c \
  mpd-shell.h \
  mpd-shell-defines.h \
  mpd-storage-device.c \
  mpd-storage-device.h \
  mpd-storage-device-tile.c \
  mpd-storage-device-tile.h \
  mpd-text.h \
  mpd-volume-tile.c \
  mpd-volume-tile.h \
  $(NULL)

-include $(top_srcdir)/git.mk


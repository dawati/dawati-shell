
AM_CFLAGS = \
  $(MPD_CFLAGS) \
  -I$(top_srcdir) \
  -I$(top_srcdir)/src \
  -DPKGICONDIR=\"$(pkgdatadir)/icons\" \
  -DPKGTHEMEDIR=\"$(PKGTHEMEDIR)\" \
  -DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
  $(NULL)

LDADD = \
  $(MPD_LIBS) \
  $(NULL)

noinst_PROGRAMS = \
  test-battery-device \
  test-battery-icon \
  test-conf \
  test-display-device \
  test-disk-tile \
  test-folder-button \
  test-folder-tile \
  test-storage-device \
  test-storage-device-tile \
  $(NULL)

test_battery_device_SOURCES = \
  test-battery-device.c \
  $(top_srcdir)/src/mpd-battery-device.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(NULL)

test_battery_icon_SOURCES = \
  test-battery-icon.c \
  $(top_srcdir)/src/mpd-battery-icon.c \
  $(NULL)

test_conf_SOURCES = \
  test-conf.c \
  $(top_srcdir)/src/mpd-conf.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(NULL)

test_display_device_LDADD = \
  $(MPD_LIBS) \
  $(top_builddir)/gpm/libgpm.la \
  $(NULL)

test_display_device_SOURCES = \
  test-display-device.c \
  $(top_srcdir)/src/mpd-conf.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(top_srcdir)/src/mpd-display-device.c \
  $(NULL)

test_disk_tile_SOURCES = \
  test-disk-tile.c \
  $(top_srcdir)/src/mpd-storage-device.c \
  $(top_srcdir)/src/mpd-disk-tile.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(NULL)

test_folder_button_SOURCES = \
  test-folder-button.c \
  $(top_srcdir)/src/mpd-folder-button.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(NULL)

test_folder_tile_SOURCES = \
  test-folder-tile.c \
  $(top_srcdir)/src/mpd-folder-button.c \
  $(top_srcdir)/src/mpd-folder-tile.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(NULL)

test_storage_device_SOURCES = \
  test-storage-device.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(top_srcdir)/src/mpd-storage-device.c \
  $(NULL)

test_storage_device_tile_SOURCES = \
  test-storage-device-tile.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(top_srcdir)/src/mpd-storage-device.c \
  $(top_srcdir)/src/mpd-storage-device-tile.c \
  $(NULL)

-include $(top_srcdir)/git.mk


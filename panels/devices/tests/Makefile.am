
AM_CFLAGS = \
  $(MPD_CFLAGS) \
  -I$(top_srcdir) \
  -I$(top_srcdir)/src \
  -DPKGICONDIR=\"$(pkgdatadir)/icons\" \
  -DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
  $(NULL)

LDADD = \
  $(MPD_LIBS) \
  $(NULL)

noinst_PROGRAMS = \
  test-battery-device \
  test-folder-button \
  $(NULL)

test_battery_device_SOURCES = \
  test-battery-device.c \
  $(top_srcdir)/src/mpd-battery-device.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(NULL)

test_disk_tile_LDADD = \
  $(top_builddir)/gdu/libgdu.la \
  $(MPD_LIBS) \
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

test_storage_device_LDADD = \
  $(top_builddir)/gdu/libgdu.la \
  $(MPD_LIBS) \
  $(NULL)

test_storage_device_SOURCES = \
  test-storage-device.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(top_srcdir)/src/mpd-storage-device.c \
  $(NULL)

test_storage_device_tile_LDADD = \
  $(top_builddir)/gdu/libgdu.la \
  $(MPD_LIBS) \
  $(NULL)

test_storage_device_tile_SOURCES = \
  test-storage-device-tile.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(top_srcdir)/src/mpd-storage-device.c \
  $(top_srcdir)/src/mpd-storage-device-tile.c \
  $(NULL)

if HAVE_UDISKS
  AM_CFLAGS += -DGDU_API_IS_SUBJECT_TO_CHANGE
  noinst_PROGRAMS += \
    test-disk-tile \
    test-storage-device \
    test-storage-device-tile \
    $(NULL)
endif

-include $(top_srcdir)/git.mk



AM_CFLAGS = \
	$(MPD_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
	$(NULL)

LDADD = \
	$(MPD_LIBS) \
	$(NULL)

noinst_PROGRAMS = \
	test-battery-device \
	test-disk-device \
	test-disk-tile \
	test-folder-button \
	test-idle-manager \
	$(NULL)

test_battery_device_SOURCES = \
	test-battery-device.c \
	$(top_srcdir)/src/mpd-battery-device.c \
	$(NULL)

test_disk_device_SOURCES = \
	test-disk-device.c \
	$(top_srcdir)/src/mpd-disk-device.c \
	$(NULL)

test_disk_tile_SOURCES = \
	test-disk-tile.c \
	$(top_srcdir)/src/mpd-disk-device.c \
	$(top_srcdir)/src/mpd-disk-tile.c \
	$(NULL)

test_folder_button_SOURCES = \
	test-folder-button.c \
	$(top_srcdir)/src/mpd-folder-button.c \
	$(NULL)

test_idle_manager_LDADD = \
	$(top_builddir)/egg-idletime/libegg-idletime.la \
	$(MPD_LIBS) \
	$(NULL)

test_idle_manager_SOURCES = \
	test-idle-manager.c \
	$(top_srcdir)/src/mpd-idle-manager.c \
	$(NULL)


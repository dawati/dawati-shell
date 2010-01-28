
AM_CFLAGS = \
	$(MPD_CFLAGS) \
	-I$(top_srcdir)/src \
	-DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
	$(NULL)

LDADD = $(MPD_LIBS)

noinst_PROGRAMS = \
	test-battery-device \
	$(NULL)

test_battery_device_SOURCES = \
	test-battery-device.c \
	$(top_srcdir)/src/mpd-battery-device.c \
	$(NULL)


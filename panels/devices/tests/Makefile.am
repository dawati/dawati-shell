
AM_CFLAGS = \
	$(DEVICES_CFLAGS) \
	-I$(top_srcdir)/src \
	$(NULL)

LDADD = $(DEVICES_LIBS)

noinst_PROGRAMS = \
	test-battery-device \
	$(NULL)

test_battery_device_SOURCES = \
	test-battery-device.c \
	$(top_srcdir)/src/mpd-battery-device.c \
	$(NULL)


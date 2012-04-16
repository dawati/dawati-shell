AM_CFLAGS = \
	$(PANEL_POWER_ICON_CFLAGS) \
	-I$(top_srcdir)/panels/power-icon/src \
	-DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
	-I$(top_srcdir)/panels/devices/src \
	-I$(top_srcdir)/panels/power-icon/src \
	$(NULL)

tools = \
	test-battery-notification \
	test-shutdown-notification \
	$(NULL)

if INSTALL_TEST_TOOLS
testsdir = $(libexecdir)/dawati-shell/tests
tests_PROGRAMS = $(tools)
else
noinst_PROGRAMS = $(tools)
endif

LDADD = \
	$(PANEL_POWER_ICON_LIBS) \
	$(NULL)

test_battery_notification_SOURCES = \
	test-battery-notification.c \
	$(top_srcdir)/panels/power-icon/src/mpd-global-key.c \
	$(top_srcdir)/panels/power-icon/src/mpd-power-icon.c \
	$(top_srcdir)/panels/power-icon/src/mpd-shutdown-notification.c \
	$(top_srcdir)/panels/devices/src/mpd-battery-device.c \
	$(top_srcdir)/panels/devices/src/mpd-gobject.c \
	$(NULL)

test_shutdown_notification_SOURCES = \
	test-shutdown-notification.c \
	$(top_srcdir)/panels/power-icon/src/mpd-shutdown-notification.c \
	$(NULL)

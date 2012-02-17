AM_CFLAGS = \
	$(PANEL_POWER_ICON_CFLAGS) \
	-I$(top_srcdir)/panels/power-icon/src \
	-DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
	-I$(top_srcdir)/panels/devices/src \
	-I$(top_srcdir)/panels/power-icon/src \
	$(NULL)

tools = \
	test-battery-notification \
	test-brightness-keys \
	test-idle-manager \
	test-shutdown-notification \
	test-xbacklight \
	$(NULL)

if INSTALL_TEST_TOOLS
	pkglib_PROGRAMS = $(tools)
else
	noinst_PROGRAMS = $(tools)
endif

LDADD = \
	$(PANEL_POWER_ICON_LIBS) \
	$(top_builddir)/egg-console-kit/libegg-console-kit.la \
	$(NULL)

test_battery_notification_LDADD = \
	$(PANEL_POWER_ICON_LIBS) \
	$(top_builddir)/egg-console-kit/libegg-console-kit.la \
	$(top_builddir)/gpm/libgpm.la \
	$(NULL)

test_battery_notification_SOURCES = \
	test-battery-notification.c \
	$(top_srcdir)/panels/power-icon/src/mpd-global-key.c \
	$(top_srcdir)/panels/power-icon/src/mpd-idle-manager.c \
	$(top_srcdir)/panels/power-icon/src/mpd-lid-device.c \
	$(top_srcdir)/panels/power-icon/src/mpd-power-icon.c \
	$(top_srcdir)/panels/power-icon/src/mpd-shutdown-notification.c \
	$(top_srcdir)/panels/devices/src/mpd-battery-device.c \
	$(top_srcdir)/panels/devices/src/mpd-conf.c \
	$(top_srcdir)/panels/devices/src/mpd-display-device.c \
	$(top_srcdir)/panels/devices/src/mpd-gobject.c \
	$(NULL)

test_brightness_keys_SOURCES = \
	test-brightness-keys.c \
	$(top_srcdir)/panels/power-icon/src/mpd-global-key.c \
	$(NULL)

test_idle_manager_SOURCES = \
	test-idle-manager.c \
	$(top_srcdir)/panels/power-icon/src/mpd-idle-manager.c \
	$(top_srcdir)/panels/devices/src/mpd-conf.c \
	$(top_srcdir)/panels/devices/src/mpd-gobject.c \
	$(NULL)

test_shutdown_notification_SOURCES = \
	test-shutdown-notification.c \
	$(top_srcdir)/panels/power-icon/src/mpd-shutdown-notification.c \
	$(NULL)

test_xbacklight_LDADD = \
	$(PANEL_POWER_ICON_LIBS) \
	$(top_builddir)/gpm/libgpm.la \
	$(NULL)

test_xbacklight_SOURCES = \
	test-xbacklight.c \
	$(NULL)

-include $(top_srcdir)/git.mk

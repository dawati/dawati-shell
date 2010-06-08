
AM_CFLAGS = \
  $(MPI_CFLAGS) \
  -I$(top_srcdir) \
  -I$(top_srcdir)/power-icon/src \
  -I$(top_srcdir)/src \
  -DPKGTHEMEDIR=\"$(PKGTHEMEDIR)/power-icon\" \
  -DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
  $(NULL)

noinst_PROGRAMS = \
  test-battery-notification \
  test-idle-manager \
  test-shutdown-notification \
  test-xbacklight \
  $(NULL)

LDADD = \
  $(top_builddir)/egg-console-kit/libegg-console-kit.la \
  $(MPI_LIBS) \
  $(NULL)

test_battery_notification_LDADD = \
  $(top_builddir)/egg-console-kit/libegg-console-kit.la \
  $(top_builddir)/gpm/libgpm.la \
  $(MPI_LIBS) \
  $(NULL)

test_battery_notification_SOURCES = \
  test-battery-notification.c \
  $(top_srcdir)/power-icon/src/mpd-global-key.c \
  $(top_srcdir)/power-icon/src/mpd-idle-manager.c \
  $(top_srcdir)/power-icon/src/mpd-lid-device.c \
  $(top_srcdir)/power-icon/src/mpd-power-icon.c \
  $(top_srcdir)/power-icon/src/mpd-shutdown-notification.c \
  $(top_srcdir)/src/mpd-battery-device.c \
  $(top_srcdir)/src/mpd-display-device.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(NULL)

test_idle_manager_SOURCES = \
  test-idle-manager.c \
  $(top_srcdir)/power-icon/src/mpd-idle-manager.c \
  $(top_srcdir)/src/mpd-gobject.c \
  $(NULL)

test_shutdown_notification_SOURCES = \
  test-shutdown-notification.c \
  $(top_srcdir)/power-icon/src/mpd-shutdown-notification.c \
  $(NULL)

test_xbacklight_LDADD = \
  $(top_builddir)/gpm/libgpm.la \
  $(MPI_LIBS) \
  $(NULL)

test_xbacklight_SOURCES = \
  test-xbacklight.c \
  $(NULL)

-include $(top_srcdir)/git.mk



AM_CFLAGS = \
	$(MPD_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/power-icon/src \
	-DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
	$(NULL)

noinst_PROGRAMS = \
	test-idle-manager \
	$(NULL)

test_idle_manager_LDADD = \
	$(top_builddir)/egg-idletime/libegg-idletime.la \
	$(MPD_LIBS) \
	$(NULL)

test_idle_manager_SOURCES = \
	test-idle-manager.c \
	$(top_srcdir)/power-icon/src/mpd-idle-manager.c \
	$(NULL)


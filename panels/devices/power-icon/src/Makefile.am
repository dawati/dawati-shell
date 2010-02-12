
AM_CFLAGS = \
	$(MPD_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPKGTHEMEDIR=\"$(PKGTHEMEDIR)/power-icon\" \
	-DI_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE \
	$(NULL)

libexec_PROGRAMS = moblin-power-icon

moblin_power_icon_LDADD = \
	$(MPD_LIBS) \
  $(NULL)

moblin_power_icon_SOURCES = \
	$(top_srcdir)/src/mpd-battery-device.c \
	moblin-power-icon.c \
	$(NULL)


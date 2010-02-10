
AM_CFLAGS = \
	$(MPD_CFLAGS) \
	-I$(top_srcdir) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPKGTHEMEDIR=\"$(pkgdatadir)/theme\" \
	$(NULL)

libexec_PROGRAMS = moblin-power-icon

moblin_power_icon_LDADD = \
	$(MPD_LIBS) \
  $(NULL)

moblin_power_icon_SOURCES = \
	moblin-power-icon.c \
	$(NULL)


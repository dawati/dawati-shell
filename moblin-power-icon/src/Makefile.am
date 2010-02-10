
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

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.power.service.in
service_DATA = org.moblin.UX.Shell.Panels.power.service

EXTRA_DIST = $(service_in_files)

DISTCLEANFILES = org.moblin.UX.Shell.Panels.power.service

org.moblin.UX.Shell.Panels.power.service: org.moblin.UX.Shell.Panels.power.service.in $(top_builddir)/config.log
	$(AM_V_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@


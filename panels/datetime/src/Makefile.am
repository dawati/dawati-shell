QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(DATETIME_CFLAGS) \
	$(MPL_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\" \
	-Wall -Werror -g3 -O0 -ggdb

moblin_panel_date_time_LDADD = $(DATETIME_LIBS) $(MPL_LIBS)

libexec_PROGRAMS = moblin-panel-date-time

moblin_panel_date_time_SOURCES = moblin-panel-datetime.c

servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.datetime.service.in
service_DATA = org.moblin.UX.Shell.Panels.datetime.service

#moblin_panel_date_time_LDFLAGS = \
#			-Wl,--export-dynamic

DISTCLEANFILES = \
	org.moblin.UX.Shell.Panels.datetime.service

EXTRA_DIST = $(service_in_files) 

org.moblin.UX.Shell.Panels.datetime.service: org.moblin.UX.Shell.Panels.datetime.service.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

#CLEANFILES = *~ moblin-panel-date-time.desktop


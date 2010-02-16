QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(DATETIME_CFLAGS) \
	$(MPL_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(pkgdatadir)/nbtk.cache\" \
	-DTHEMEDIR=\"$(pkgdatadir)/theme\" \
	-Wall -g3 -O0 -ggdb

moblin_panel_datetime_LDADD = $(DATETIME_LIBS) $(MPL_LIBS)
moblin_alarm_notify_LDADD =  $(DATETIME_LIBS) $(MPL_LIBS)

libexec_PROGRAMS = moblin-panel-datetime

bin_PROGRAMS = moblin-alarm-notify

moblin_panel_datetime_SOURCES = moblin-panel-date-time.c	\
				 mnp-world-clock.c	\
				 mnp-world-clock.h	\
				 mnp-utils.c 		\
				 mnp-utils.h		\
				 mnp-button-item.c	\
				 mnp-button-item.h	\
				 system-timezone.c	\
				 system-timezone.h	\
				 mnp-clock-tile.c	\
				 mnp-clock-tile.h	\
				 mnp-clock-area.c	\
				 mnp-clock-area.h	\
				 mnp-datetime.c		\
				 mnp-datetime.h		\
				 mnp-alarms.c		\
				 mnp-alarms.h		\
				 mnp-alarm-tile.c	\
				 mnp-alarm-tile.h	\
				 mnp-alarm-dialog.c	\
				 mnp-alarm-dialog.h	\
				 mx-spin-entry.c	\
				 mx-spin-entry.h	\
				 mnp-alarm-utils.h	

moblin_alarm_notify_SOURCES = moblin-alarm-notify.c 	\
			      mnp-alarm-manager.c	\
			      mnp-alarm-manager.h	\
			      mnp-alarm-instance.c	\
			      mnp-alarm-instance.h	\
			      mnp-alarm-utils.h


servicedir = $(datadir)/dbus-1/services
service_in_files = org.moblin.UX.Shell.Panels.datetime.service.in
service_DATA = org.moblin.UX.Shell.Panels.datetime.service

schemadir   = $(GCONF_SCHEMA_FILE_DIR)
schema_in_files = date-time.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

#moblin_panel_date_time_LDFLAGS = \
#			-Wl,--export-dynamic

DISTCLEANFILES = \
	org.moblin.UX.Shell.Panels.datetime.service

EXTRA_DIST = $(service_in_files) \
	     $(schema_in_files) 	     

org.moblin.UX.Shell.Panels.datetime.service: org.moblin.UX.Shell.Panels.datetime.service.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@



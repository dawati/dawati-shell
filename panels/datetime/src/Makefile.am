QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CFLAGS = \
	$(PANEL_DATETIME_CFLAGS) \
	$(PENGE_CFLAGS) \
	-I$(top_srcdir) \
	\
	-DMYZONETHEMEDIR=\"$(PANEL_MYZONE_THEMEDIR)\" \
	-Wall -g3 -O0 -ggdb \
	$(NULL)

dawati_panel_datetime_LDADD = \
	$(PANEL_DATETIME_LIBS) \
	$(PENGE_LIBS) \
	$(top_builddir)/penge/libpenge.la \
	$(NULL)

dawati_alarm_notify_LDADD = \
	$(LIBMPL_LIBS) \
	$(PANEL_DATETIME_LIBS) \
	$(NULL)

libexec_PROGRAMS = dawati-panel-datetime

#bin_PROGRAMS = dawati-alarm-notify

dawati_panel_datetime_SOURCES = \
	dawati-panel-date-time.c \
	mnp-world-clock.c \
	mnp-world-clock.h \
	mnp-utils.c \
	mnp-utils.h \
	mnp-button-item.c \
	mnp-button-item.h \
	system-timezone.c \
	system-timezone.h \
	mnp-clock-tile.c \
	mnp-clock-tile.h \
	mnp-clock-area.c \
	mnp-clock-area.h \
	mnp-datetime.c \
	mnp-datetime.h \
	mnp-shell.c \
	mnp-shell.h \
	$(NULL)


#				 mnp-alarms.c		\
#				 mnp-alarms.h		\
#				 mnp-alarm-tile.c	\
#				 mnp-alarm-tile.h	\
#				 mnp-alarm-dialog.c	\
#				 mnp-alarm-dialog.h	\
#				 mx-spin-entry.c	\
#				 mx-spin-entry.h	\
#				 mnp-alarm-utils.h

dawati_alarm_notify_SOURCES = \
	dawati-alarm-notify.c \
	mnp-alarm-manager.c \
	mnp-alarm-manager.h \
	mnp-alarm-instance.c \
	mnp-alarm-instance.h \
	mnp-alarm-utils.h \
	$(NULL)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.datetime.service.in
service_DATA = com.dawati.UX.Shell.Panels.datetime.service

schemadir   = $(GCONF_SCHEMA_FILE_DIR)
schema_in_files = date-time.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

#dawati_panel_date_time_LDFLAGS = \
#			-Wl,--export-dynamic

DISTCLEANFILES = \
	com.dawati.UX.Shell.Panels.datetime.service \
	date-time.schemas \
	$(NULL)

EXTRA_DIST = $(service_in_files) \
	     $(schema_in_files)

com.dawati.UX.Shell.Panels.datetime.service: com.dawati.UX.Shell.Panels.datetime.service.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@



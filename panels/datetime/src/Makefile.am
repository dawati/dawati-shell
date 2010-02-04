
bin_PROGRAMS = moblin-panel-date-time
moblin_panel_date_time_SOURCES = main.c

desktopentry_DATA = moblin-panel-date-time.desktop

AM_CFLAGS = @MOBLIN_PANEL_DATE_TIME_CFLAGS@ -Wall -Werror -g3 -O0 -ggdb -DPKGDATADIR="\"$(pkgdatadir)\""
AM_LDFLAGS = @MOBLIN_PANEL_DATE_TIME_LIBS@ 


#moblin_panel_date_time_LDFLAGS = \
#			-Wl,--export-dynamic

#moblin_panel_date_time_LDADD = $(ANJAL_LIBS) 
#

#Example for copying directory to project data directory on "make install"
#install-data-hook:
#	$(MKDIR_P) ${pkgdatadir}
#	cp -R <dir>/* ${pkgdatadir}

#EXTRA_DIST = 

CLEANFILES = *~ moblin-panel-date-time.desktop


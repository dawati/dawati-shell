lib_LTLIBRARIES = libpenge.la
libpenge_ladir = $(includedir)/penge/penge

libpenge_la_HEADERS = penge-people-pane.h \
		   penge-people-tile.h \
		   penge-flickr-tile.h \
		   penge-recent-files-pane.h \
		   penge-recent-file-tile.h \
		   penge-date-tile.h \
		   penge-event-tile.h \
		   penge-events-pane.h \
		   penge-calendar-pane.h \
		   penge-grid-view.h \
		   penge-utils.h

libpenge_la_SOURCES = \
		      $(libpenge_la_HEADERS) \
		      penge-people-pane.c \
		      penge-people-tile.c \
		      penge-flickr-tile.c \
		      penge-twitter-tile.c \
		      penge-recent-files-pane.c \
		      penge-recent-file-tile.c \
		      penge-date-tile.c \
		      penge-event-tile.c \
		      penge-events-pane.c \
		      penge-calendar-pane.c \
		      penge-grid-view.c \
		      penge-utils.c
libpenge_la_CFLAGS = -Wall $(GLIB_CFLAGS) $(NBTK_CFLAGS) $(JANA_ECAL_CFLAGS) \
		     $(MOJITO_CLIENT_CFLAGS) $(GTK_CFLAGS) \
		     -DPKG_DATADIR=\"$(pkgdatadir)\"
libpenge_la_LIBADD = $(GLIB_LIBS) $(NBTK_LIBS) $(JANA_ECAL_LIBS) \
		     $(MOJITO_CLIENT_LIBS) $(GTK_LIBS)


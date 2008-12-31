lib_LTLIBRARIES = libpenge.la
libpenge_la_SOURCES = \
		      penge-people-pane.h penge-people-pane.c \
		      penge-people-tile.h penge-people-tile.c \
		      penge-flickr-tile.h penge-flickr-tile.c \
		      penge-twitter-tile.h penge-twitter-tile.c \
		      penge-recent-files-pane.h penge-recent-files-pane.c \
		      penge-recent-file-tile.h penge-recent-file-tile.c \
		      penge-date-tile.h penge-date-tile.c \
		      penge-event-tile.h penge-event-tile.c \
		      penge-events-pane.h penge-events-pane.c \
		      penge-utils.h penge-utils.c
libpenge_la_CFLAGS = -Wall $(GLIB_CFLAGS) $(NBTK_CFLAGS) $(JANA_ECAL_CFLAGS) \
		     $(MOJITO_CLIENT_CFLAGS) $(GTK_CFLAGS)
libpenge_la_LIBADD = $(GLIB_LIBS) $(NBTK_LIBS) $(JANA_ECAL_LIBS) \
		     $(MOJITO_CLIENT_LIBS) $(GTK_LIBS)


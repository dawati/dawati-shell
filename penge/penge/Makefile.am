lib_LTLIBRARIES = libpenge.la
libpenge_ladir = $(includedir)/penge/penge

libpenge_la_HEADERS = penge-people-pane.h \
		   penge-people-tile.h \
		   penge-flickr-tile.h \
		   penge-twitter-tile.h \
		   penge-recent-files-pane.h \
		   penge-recent-file-tile.h \
		   penge-date-tile.h \
		   penge-event-tile.h \
		   penge-events-pane.h \
		   penge-calendar-pane.h \
		   penge-grid-view.h \
		   penge-utils.h \
		   penge-magic-texture.h \
		   penge-view-background.h

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
		      penge-utils.c \
		      penge-magic-texture.c \
		      penge-view-background.c
libpenge_la_CFLAGS = -Wall $(PENGE_CFLAGS) \
		     -DPKG_DATADIR=\"$(pkgdatadir)\"
libpenge_la_LIBADD = $(PENGE_LIBS)


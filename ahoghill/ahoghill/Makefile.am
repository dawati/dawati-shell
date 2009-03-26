noinst_LTLIBRARIES = libahoghill.la

libahoghill_headers = ahoghill-grid-view.h	\
	ahoghill-media-art.h			\
	ahoghill-media-tile.h			\
	ahoghill-results-pane.h			\
	ahoghill-search-pane.h

libahoghill_la_SOURCES = $(libahoghill_headers) \
	ahoghill-grid-view.c \
	ahoghill-media-art.c \
	ahoghill-media-tile.c \
	ahoghill-results-pane.c \
	ahoghill-search-pane.c
AM_CFLAGS = -Wall -fPIC $(AHOGHILL_CFLAGS) \
	    -DPKG_DATADIR=\"$(pkgdatadir)\"
AM_LDFLAGS = $(AHOGHILL_LIBS)


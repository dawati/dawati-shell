noinst_LTLIBRARIES = libahoghill.la

libahoghill_headers = ahoghill-grid-view.h	\
	ahoghill-media-art.h			\
	ahoghill-media-tile.h			\
	ahoghill-results-model.h		\
	ahoghill-results-pane.h			\
	ahoghill-results-table.h		\
	ahoghill-search-pane.h			\
	nbtk-fixed.h

libahoghill_la_SOURCES = $(libahoghill_headers) \
	ahoghill-grid-view.c \
	ahoghill-media-art.c \
	ahoghill-media-tile.c \
	ahoghill-results-model.c \
	ahoghill-results-pane.c \
	ahoghill-results-table.c \
	ahoghill-search-pane.c \
	nbtk-fixed.c
AM_CFLAGS = -Wall -fPIC $(AHOGHILL_CFLAGS) \
	    -DPKG_DATADIR=\"$(pkgdatadir)\" -I$(top_srcdir)
AM_LDFLAGS = $(AHOGHILL_LIBS)


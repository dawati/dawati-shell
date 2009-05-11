noinst_LTLIBRARIES = libahoghill.la

libahoghill_headers = ahoghill-grid-view.h	\
	ahoghill-media-art.h			\
	ahoghill-media-tile.h			\
	ahoghill-play-button.h			\
	ahoghill-playlist.h			\
	ahoghill-playlist-header.h		\
	ahoghill-queue-list.h			\
	ahoghill-queue-tile.h			\
	ahoghill-results-model.h		\
	ahoghill-results-pane.h			\
	ahoghill-results-table.h		\
	ahoghill-search-pane.h			\
	nbtk-fixed.h

libahoghill_la_SOURCES = $(libahoghill_headers) \
	ahoghill-grid-view.c \
	ahoghill-media-art.c \
	ahoghill-media-tile.c \
	ahoghill-play-button.c \
	ahoghill-playlist.c \
	ahoghill-playlist-header.c \
	ahoghill-queue-list.c \
	ahoghill-queue-tile.c \
	ahoghill-results-model.c \
	ahoghill-results-pane.c \
	ahoghill-results-table.c \
	ahoghill-search-pane.c \
	nbtk-fixed.c
AM_CFLAGS = -Wall -fPIC @AHOGHILL_CFLAGS@ \
	    -DPKG_DATADIR=\"$(pkgdatadir)\" -I$(top_srcdir) \
	    -I$(top_srcdir)/ahoghill
AM_LDFLAGS = @AHOGHILL_LIBS@


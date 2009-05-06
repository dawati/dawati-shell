lib_LTLIBRARIES = libanerley.la

libanerley_la_HEADERS = anerley-tp-feed.h \
			anerley-feed.h \
			anerley-item.h \
			anerley-tp-item.h \
			anerley-tile.h \
			anerley-simple-grid-view.h \
			anerley-feed-model.h \
			anerley-tile-view.h \
			anerley-aggregate-feed.h \
			anerley-aggregate-tp-feed.h
libanerley_la_SOURCES = anerley-tp-feed.c \
			anerley-feed.c \
			anerley-item.c \
			anerley-tp-item.c \
			anerley-tile.c \
			anerley-simple-grid-view.c \
			anerley-feed-model.c \
			anerley-tile-view.c \
			anerley-aggregate-feed.c \
			anerley-aggregate-tp-feed.c \
			$(libanerley_la_HEADERS)
libanerley_ladir = $(includedir)/anerley/anerley

libanerley_la_CFLAGS = $(GLIB_CFLAGS) $(TELEPATHY_CFLAGS) $(MC_CFLAGS) \
		       $(NBTK_CFLAGS) \
		       -DPKG_DATA_DIR=\"$(pkgdatadir)\"
libanerley_la_LIBADD = $(GLIB_LIBS) $(TELEPATHY_LIBS) $(MC_LIBS) \
		       $(NBTK_LIBS)


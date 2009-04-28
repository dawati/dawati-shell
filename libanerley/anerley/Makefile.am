noinst_LTLIBRARIES = libanerley.la

libanerley_la_SOURCES = anerley-tp-feed.c \
			anerley-tp-feed.h \
			anerley-feed.c \
			anerley-feed.h \
			anerley-item.c \
			anerley-item.h \
			anerley-tp-item.c \
			anerley-tp-item.h \
			anerley-tile.c \
			anerley-tile.h 
libanerley_la_CFLAGS = $(GLIB_CFLAGS) $(TELEPATHY_CFLAGS) $(MC_CFLAGS) \
		       $(NBTK_CFLAGS) \
		       -DPKG_DATA_DIR=\"$(pkgdatadir)\"
libanerley_la_LIBADD = $(GLIB_LIBS) $(TELEPATHY_LIBS) $(MC_LIBS) \
		       $(NBTK_LIBS)


lib_LTLIBRARIES = libanerley.la

%.c: %.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --body --prefix=anerley_marshal $< > $@

%.h: %.list Makefile.am
	$(QUIET_GEN)glib-genmarshal --header --prefix=anerley_marshal $< > $@

EXTRA_DIST = anerley-marshals.list
BUILT_SOURCES = anerley-marshals.c anerley-marshals.h

libanerley_la_HEADERS = anerley-tp-feed.h \
			anerley-feed.h \
			anerley-item.h \
			anerley-tp-item.h \
			anerley-tile.h \
			anerley-simple-grid-view.h \
			anerley-feed-model.h \
			anerley-tile-view.h \
			anerley-aggregate-feed.h \
			anerley-aggregate-tp-feed.h \
			anerley-econtact-item.h \
			anerley-ebook-feed.h \
			anerley-tp-observer.h \
			anerley-tp-monitor-feed.h \
			anerley-presence-chooser.h \
			anerley-compact-tile.h \
			anerley-compact-tile-view.h
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
			anerley-econtact-item.c \
			anerley-ebook-feed.c \
			anerley-tp-observer.c \
			anerley-tp-monitor-feed.c \
			anerley-presence-chooser.c \
			anerley-main.c \
			anerley-compact-tile.c \
			anerley-compact-tile-view.c \
			$(libanerley_la_HEADERS) \
			$(BUILT_SOURCES) \
			penge-magic-texture.c \
			penge-magic-texture.h
libanerley_ladir = $(includedir)/anerley/anerley

libanerley_la_CFLAGS = -I$(top_srcdir) \
		       $(GLIB_CFLAGS) $(TELEPATHY_CFLAGS) \
		       $(MX_CFLAGS) $(EBOOK_CFLAGS) \
		       -DPKG_DATA_DIR=\"$(pkgdatadir)\" \
		       -D_GNU_SOURCE
libanerley_la_LIBADD = $(GLIB_LIBS) $(TELEPATHY_LIBS) \
		       $(MX_LIBS) $(EBOOK_LIBS)


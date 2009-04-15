noinst_PROGRAMS = test-tp-feed

AM_CFLAGS = $(GLIB_CFLAGS) $(TELEPATHY_CFLAGS) $(MC_CFLAGS) -I$(top_srcdir)
AM_LDFLAGS = $(GLIB_LIBS) $(TELEPATHY_LIBS) $(MC_LIBS) \
	     $(top_builddir)/anerley/libanerley.la

test_tp_feed_SOURCES = test-tp-feed.c


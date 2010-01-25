noinst_PROGRAMS = test-tp-feed test-simple-grid-view test-tile-view test-aggregate-tp-feed test-simple-ui test-tp-observer test-tp-monitor-feed test-presence-chooser

AM_CFLAGS = $(GLIB_CFLAGS) $(TELEPATHY_CFLAGS) $(MX_CFLAGS) \
	    -I$(top_srcdir) \
	    -DPKG_DATA_DIR=\"$(pkgdatadir)\"
AM_LDFLAGS = $(GLIB_LIBS) $(TELEPATHY_LIBS) $(MX_LIBS)

test_tp_feed_SOURCES = test-tp-feed.c
test_tp_feed_LDADD = ../anerley/libanerley.la

test_simple_grid_view_SOURCES = test-simple-grid-view.c
test_simple_grid_view_LDADD = ../anerley/libanerley.la

test_tile_view_SOURCES = test-tile-view.c
test_tile_view_LDADD = ../anerley/libanerley.la

test_aggregate_tp_feed_SOURCES = test-aggregate-tp-feed.c
test_aggregate_tp_feed_LDADD = ../anerley/libanerley.la

test_simple_ui_SOURCES = test-simple-ui.c
test_simple_ui_LDADD = ../anerley/libanerley.la

test_tp_observer_SOURCES = test-tp-observer.c
test_tp_observer_LDADD = ../anerley/libanerley.la

test_tp_monitor_feed_SOURCES = test-tp-monitor-feed.c
test_tp_monitor_feed_LDADD = ../anerley/libanerley.la

test_presence_chooser_SOURCES = test-presence-chooser.c
test_presence_chooser_LDADD = ../anerley/libanerley.la

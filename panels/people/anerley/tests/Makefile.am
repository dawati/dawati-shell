noinst_PROGRAMS = \
	test-presence-chooser \
	test-simple-grid-view \
	test-simple-ui \
	test-tile-view \
	test-tp-feed \
	test-tp-monitor-feed \
	test-tp-user-avatar \
	$(NULL)

AM_CFLAGS = $(PANEL_PEOPLE_CFLAGS) \
	    -I$(top_srcdir)/panels/people/anerley \
	    -DPKG_DATA_DIR=\"$(pkgdatadir)\"
AM_LDFLAGS = $(PANEL_PEOPLE_LIBS)

test_tp_feed_SOURCES = test-tp-feed.c
test_tp_feed_LDADD = ../anerley/libanerley.la

test_simple_grid_view_SOURCES = test-simple-grid-view.c
test_simple_grid_view_LDADD = ../anerley/libanerley.la

test_tile_view_SOURCES = test-tile-view.c
test_tile_view_LDADD = ../anerley/libanerley.la

test_simple_ui_SOURCES = test-simple-ui.c
test_simple_ui_LDADD = ../anerley/libanerley.la

test_tp_monitor_feed_SOURCES = test-tp-monitor-feed.c
test_tp_monitor_feed_LDADD = ../anerley/libanerley.la

test_presence_chooser_SOURCES = test-presence-chooser.c
test_presence_chooser_LDADD = ../anerley/libanerley.la

test_tp_user_avatar_SOURCES = test-tp-user-avatar.c
test_tp_user_avatar_LDADD = ../anerley/libanerley.la


#noinst_PROGRAMS = test-tiles test-recent-file-tiles test-date-tiles
#noinst_PROGRAMS = test-grid-view test-app-bookmarks test-time-formatting

# FIXME -- the tests are disabled because Penge cannot currently be build as
# a standalone library (it calls into the moblin_netbook_launch*() API; this
# dependency will go away with the dbus API).
noinst_PROGRAMS =
AM_CFLAGS = $(PENGE_CFLAGS) -I$(top_srcdir)/penge
AM_LDFLAGS = $(PENGE_LIBS) ../penge/libpenge.la

#test_tiles_SOURCES = test-tiles.c
#test_recent_file_tiles_SOURCES = test-recent-file-tiles.c
#test_date_tiles_SOURCES = test-date-tiles.c
#test_people_pane_SOURCES = test-people-pane.c
#test_recent_files_pane_SOURCES = test-recent-files-pane.c
#test_events_pane_SOURCES = test-events-pane.c
#test_grid_view_SOURCES = test-grid-view.c
#test_grid_view_DEPENDENCIES = ../penge/libpenge.la

#test_grid_app_bookmarks_SOURCES = test-grid-app_bookmarks.c
#test_grid_app_bookmarks_DEPENDENCIES = ../penge/libpenge.la

#test_time_formatting_bookmarks_SOURCES = test-time-formatting.c
#test_time_formatting_bookmarks_DEPENDENCIES = ../penge/libpenge.la

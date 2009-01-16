#noinst_PROGRAMS = test-tiles test-recent-file-tiles test-date-tiles
noinst_PROGRAMS = test-people-pane test-recent-files-pane test-events-pane \
		  test-grid-view
AM_CFLAGS = $(PENGE_CFLAGS) -I$(top_srcdir)/penge
AM_LDFLAGS = $(PENGE_LIBS) ../penge/libpenge.la

#test_tiles_SOURCES = test-tiles.c
#test_recent_file_tiles_SOURCES = test-recent-file-tiles.c
#test_date_tiles_SOURCES = test-date-tiles.c
test_people_pane_SOURCES = test-people-pane.c
test_recent_files_pane_SOURCES = test-recent-files-pane.c
test_events_pane_SOURCES = test-events-pane.c
test_grid_view_SOURCES = test-grid-view.c

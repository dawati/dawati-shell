#noinst_PROGRAMS = test-tiles test-recent-file-tiles test-date-tiles
noinst_PROGRAMS = test-people-pane test-recent-files-pane test-events-pane
AM_CFLAGS = $(GLIB_CFLAGS) $(NBTK_CFLAGS) $(JANA_ECAL_CFLAGS) $(GTK_CFLAGS) \
	    $(MOJITO_CLIENT_CFLAGS) \
	    -I$(top_srcdir)/penge
AM_LDFLAGS = $(GLIB_LIBS) $(NBTK_LIBS) $(JANA_ECAL_LIBS) $(GTK_LIBS) \
	     $(MOJITO_CLIENT_CFLAGS) \
	     ../penge/libpenge.la
#test_tiles_SOURCES = test-tiles.c
#test_recent_file_tiles_SOURCES = test-recent-file-tiles.c
#test_date_tiles_SOURCES = test-date-tiles.c
test_people_pane_SOURCES = test-people-pane.c
test_recent_files_pane_SOURCES = test-recent-files-pane.c
test_events_pane_SOURCES = test-events-pane.c

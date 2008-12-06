noinst_PROGRAMS = test-tiles test-recent-file-tiles

AM_CFLAGS = $(GLIB_CFLAGS) $(NBTK_CFLAGS) $(JANA_ECAL_CFLAGS) $(GTK_CFLAGS) \
	    -I$(top_srcdir)/penge
AM_LDFLAGS = $(GLIB_LIBS) $(NBTK_LIBS) $(JANA_ECAL_LIBS) $(GTK_LIBS) \
	     ../penge/libpenge.la
test_tiles_SOURCES = test-tiles.c
test_recent_file_tiles_SOURCES = test-recent-file-tiles.c

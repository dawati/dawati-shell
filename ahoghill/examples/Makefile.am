#noinst_PROGRAMS = test-tiles test-recent-file-tiles test-date-tiles
noinst_PROGRAMS = test-grid-view
AM_CFLAGS = $(AHOGHILL_CFLAGS) -I$(top_srcdir)/ahoghill \
	-DPKG_DATADIR=\"$(pkgdatadir)\"
AM_LDFLAGS = $(AHOGHILL_LIBS) ../ahoghill/libahoghill.la

test_grid_view_SOURCES = test-grid-view.c mnb-entry.c mnb-entry.h
test_grid_view_DEPENDENCIES = ../ahoghill/libahoghill.la

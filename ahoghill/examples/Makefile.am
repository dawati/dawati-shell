noinst_PROGRAMS = test-grid-view test-fixed
AM_CFLAGS = $(AHOGHILL_CFLAGS) -I$(top_srcdir)/ahoghill \
	-DPKG_DATADIR=\"$(pkgdatadir)\"
AM_LDFLAGS = $(AHOGHILL_LIBS) ../ahoghill/libahoghill.la

test_grid_view_SOURCES = test-grid-view.c mnb-entry.c mnb-entry.h
test_grid_view_DEPENDENCIES = ../ahoghill/libahoghill.la

test_fixed_SOURCES = test-fixed.c
test_fixed_DEPENDENCIES = ../ahoghill/libahoghill.la

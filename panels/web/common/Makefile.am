AM_CPPFLAGS = \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DMWB_VERSION_STRING=\"$(MWB_VERSION)\" \
	$(NBTK_CFLAGS) \
	$(GTK_CFLAGS) \
	-Wall \
	-I$(top_srcdir)/common

noinst_LIBRARIES = libcommon.a

libcommon_a_SOURCES = \
	mwb-ac-list.c \
	mwb-ac-list.h \
	mwb-radical-bar.c \
	mwb-radical-bar.h \
	mwb-separator.c \
	mwb-separator.h \
	mwb-spindle.c \
	mwb-spindle.h \
	mwb-utils.c \
	mwb-utils.h

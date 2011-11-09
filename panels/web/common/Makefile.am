AM_CPPFLAGS = \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DMWB_VERSION_STRING=\"$(MWB_VERSION)\" \
	$(MX_CFLAGS) \
	$(GTK_CFLAGS) \
	-Wall \
	-fno-exceptions -fno-rtti \
	-I$(top_srcdir)/common

noinst_LIBRARIES = libcommon.a

libcommon_a_SOURCES = \
	mwb-ac-list.cc \
	mwb-ac-list.h \
	mwb-radical-bar.cc \
	mwb-radical-bar.h \
	mwb-separator.cc \
	mwb-separator.h \
	mwb-spindle.cc \
	mwb-spindle.h \
	mwb-utils.cc \
	mwb-utils.h 

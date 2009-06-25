
ACLOCAL_AMFLAGS = -I build/autotools

SUBDIRS=build
SUBDIRS+=libmoblin-panel penge
if WITH_AHOGHILL
SUBDIRS+=ahoghill
endif
SUBDIRS+=src po data tests applications myzone pasteboard
if WITH_PEOPLE
SUBDIRS+=people
endif

EXTRA_DIST =

DISTCLEANFILES = intltool-extract intltool-merge intltool-update po/stamp-it po/.intltool-merge-cache

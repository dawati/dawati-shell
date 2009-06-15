
ACLOCAL_AMFLAGS = -I build/autotools

SUBDIRS=build
SUBDIRS+=penge
if WITH_AHOGHILL
SUBDIRS+=ahoghill
endif
SUBDIRS+=src po data libmoblin-panel tests applications

EXTRA_DIST =

DISTCLEANFILES = intltool-extract intltool-merge intltool-update po/stamp-it po/.intltool-merge-cache

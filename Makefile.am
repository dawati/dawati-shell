ACLOCAL_AMFLAGS = -I build/autotools

SUBDIRS = build

# always built
SUBDIRS += libmoblin-panel

if WITH_NETPANEL
SUBDIRS +=netpanel
endif

# extra
SUBDIRS += src po data tests

EXTRA_DIST =

DISTCLEANFILES = intltool-extract intltool-merge intltool-update po/stamp-it po/.intltool-merge-cache

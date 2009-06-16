ACLOCAL_AMFLAGS = -I build/autotools

SUBDIRS = build

# always built
SUBDIRS += libmoblin-panel penge myzone applications pasteboard

if WITH_AHOGHILL
SUBDIRS += ahoghill
endif

if WITH_PEOPLE
SUBDIRS += people
endif

if WITH_NETPANEL
SUBDIRS +=netpanel
endif

if WITH_STATUS
SUBDIRS += status
endif

# extra
SUBDIRS += src po data tests

EXTRA_DIST =

DISTCLEANFILES = intltool-extract intltool-merge intltool-update po/stamp-it po/.intltool-merge-cache

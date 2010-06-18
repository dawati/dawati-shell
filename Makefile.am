ACLOCAL_AMFLAGS = -I build/autotools

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

SUBDIRS = build

# always built
SUBDIRS += libmeego-panel

# extra
SUBDIRS += src po data tests

if HAVE_CAPPLET
  SUBDIRS += capplet
endif

EXTRA_DIST =

DISTCLEANFILES = intltool-extract intltool-merge intltool-update po/stamp-it po/.intltool-merge-cache

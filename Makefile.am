ACLOCAL_AMFLAGS = -I build/autotools

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --disable-capplet

# always built
SUBDIRS = build libdawati-panel

if USE_ECK_GPM_GVC
SUBDIRS += egg-console-kit gpm gvc
endif

if USE_PENGE_MAILME
SUBDIRS += mailme penge
endif

if USE_CARRICK_CALLOUTS
SUBDIRS += carrick callouts
endif

if HAVE_MUTTER
SUBDIRS += src
endif

# extra
SUBDIRS += po data panels

if INSTALL_TEST_TOOLS
SUBDIRS += tests
endif

if HAVE_CAPPLET
SUBDIRS += capplet
endif

EXTRA_DIST = autogen.sh

DISTCLEANFILES = intltool-extract intltool-merge intltool-update po/stamp-it po/.intltool-merge-cache

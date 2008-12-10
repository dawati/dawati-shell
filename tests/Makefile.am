
INCLUDES=@GTK_CFLAGS@ -DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\"

AM_CFLAGS = -Werror=missing-prototypes -fPIC

LDADD = $(GTK_LIBS)

noinst_PROGRAMS = \
	test-tray

test_tray_SOURCES = test-tray.c

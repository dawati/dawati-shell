
INCLUDES=@GTK_CFLAGS@ -DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\"

AM_CFLAGS = \
	$(PENGE_CFLAGS) \
	-Werror=missing-prototypes \
	-fPIC

LDADD = $(PENGE_LIBS)

noinst_PROGRAMS = \
	test-launcher-button \
	test-tray

test_launcher_button_CPPFLAGS = \
	-I$(top_srcdir)/src

test_launcher_button_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-button.c \
	test-launcher-button.c

test_tray_SOURCES = test-tray.c


AM_CPPFLAGS = \
  @MUTTER_PLUGIN_CFLAGS@ \
  -I$(top_srcdir)/src \
  -DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\"

AM_CFLAGS = \
	$(PENGE_CFLAGS) \
	-Werror=missing-prototypes \
	-fPIC

LDADD = $(PENGE_LIBS)

noinst_PROGRAMS = \
	test-launcher-button \
	test-launcher-searchbar \
	test-tray

test_launcher_button_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-button.c \
	test-launcher-button.c

test_launcher_searchbar_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-searchbar.c \
	test-launcher-searchbar.c

test_tray_SOURCES = test-tray.c

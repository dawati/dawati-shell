
AM_CPPFLAGS = \
  @MUTTER_PLUGIN_CFLAGS@ \
  -I$(top_srcdir)/penge \
  -I$(top_srcdir)/src \
  -I$(top_builddir)/src \
  -DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\"

AM_CFLAGS = \
	$(PENGE_CFLAGS) \
	-Werror=missing-prototypes \
	-fPIC

LDADD = $(PENGE_LIBS)

noinst_PROGRAMS = \
	test-entry \
	test-launcher-button \
	test-tray

test_launcher_button_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-button.c \
	test-launcher-button.c

test_entry_LDADD = \
	$(PENGE_LIBS) \
	$(MUTTER_PLUGIN_LIBS) \
	$(top_builddir)/penge/penge/libpenge.la

test_entry_SOURCES = \
	$(top_srcdir)/src/mnb-entry.c \
	test-entry.c

test_tray_SOURCES = test-tray.c

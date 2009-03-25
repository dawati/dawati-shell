
AM_CPPFLAGS = \
  $(MUTTER_PLUGIN_CFLAGS) \
  -I$(top_srcdir)/penge \
  -I$(top_srcdir)/src \
  -I$(top_builddir)/src \
  -DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\"

AM_CFLAGS = \
	-Werror=missing-prototypes

LDADD = \
	$(PENGE_LIBS) \
	$(MUTTER_PLUGIN_LIBS) \
	$(top_builddir)/penge/penge/libpenge.la

noinst_PROGRAMS = \
	test-entry \
	test-expander \
	test-launcher-button \
	test-launcher-monitor \
	test-launcher-tree \
	test-tray

test_launcher_button_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-button.c \
	test-launcher-button.c

test_entry_SOURCES = \
	$(top_srcdir)/src/mnb-entry.c \
	test-entry.c

test_expander_SOURCES = \
	$(top_srcdir)/src/mnb-expander.c \
	test-expander.c

test_launcher_monitor_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-tree.c \
	test-launcher-monitor.c

test_launcher_tree_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-tree.c \
	test-launcher-tree.c

test_tray_SOURCES = \
  test-tray.c


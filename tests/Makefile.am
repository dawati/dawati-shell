
AM_CPPFLAGS = \
  $(MUTTER_PLUGIN_CFLAGS) \
  -I$(top_srcdir)/penge \
  -I$(top_srcdir)/src \
  -I$(top_builddir)/src \
  -DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\"

AM_CFLAGS = \
	$(PENGE_CFLAGS) \
	-fPIC \
	-Werror=missing-prototypes

LDADD = \
	$(PENGE_LIBS) \
	$(MUTTER_PLUGIN_LIBS) \
	$(top_builddir)/penge/penge/libpenge.la

noinst_PROGRAMS = \
	test-entry \
	test-launcher-button \
	test-launcher-monitor \
	test-launcher-tree \
	test-tray \
	test-pasteboard \
	test-screensized \
	test-panel

test_launcher_button_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-button.c \
	test-launcher-button.c

test_entry_SOURCES = \
	$(top_srcdir)/src/mnb-entry.c \
	test-entry.c

test_launcher_monitor_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-tree.c \
	test-launcher-monitor.c

test_launcher_tree_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-tree.c \
	test-launcher-tree.c

test_tray_SOURCES = \
	test-tray.c

test_pasteboard_SOURCES = \
	$(top_srcdir)/src/mnb-clipboard-item.c \
	$(top_srcdir)/src/mnb-clipboard-store.c \
	$(top_srcdir)/src/mnb-clipboard-view.c \
	$(top_srcdir)/src/mnb-entry.c \
	$(top_builddir)/src/marshal.c \
	test-pasteboard.c

test_screensized_SOURCES = \
	test-screensized.c

test_panel_SOURCES = \
	test-panel.c
test_panel_LDADD = \
	$(MUTTER_PLUGIN_LIBS) \
	$(top_builddir)/libmnb/libmnbgtk.la

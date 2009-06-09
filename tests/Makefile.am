
%.service: %.service.in ../config.log
	sed -e "s|\@dir\@|$(abs_builddir)|" $< > $@

%.css: %.css.in ../config.log
	sed -e "s|\@pkgdatadir\@|$(pkgdatadir)|" $< > $@

AM_CPPFLAGS = \
  $(MUTTER_PLUGIN_CFLAGS) \
  -I$(top_srcdir)/penge \
  -I$(top_srcdir)/src \
  -I$(top_builddir)/src \
  -DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\" \
  -DCSS_DIR=\"$(abs_builddir)\"

AM_CFLAGS = \
	$(PENGE_CFLAGS) \
	-fPIC \
	-Werror=missing-prototypes

# FIXME -- because of various fscked dependencies any tests relying on Penge
# cannot be built (Penge calls into moblin_netbook_launch_*() functions, which
# dependency will go away when we move to the dbus API.) In the meantime,

# test-launcher: uses Penge bookmark manager; for this to work, Penge will need
#                to be a shared library.
#
# test-pasteboard: it uses some formating function from Penge utils; I just
#                c&p it into test-launcher.c

LDADD = \
	$(MUTTER_PLUGIN_LIBS)

noinst_PROGRAMS = \
	test-entry \
	test-launcher \
	test-launcher-button \
	test-launcher-monitor \
	test-launcher-tree \
	test-tray \
	test-pasteboard \
	test-screensized \
	test-panel-gtk \
	test-panel-clutter

test_launcher_LDADD = \
  ../penge/penge/libpenge.la

test_launcher_SOURCES = \
	$(top_srcdir)/src/mnb-entry.c \
	$(top_srcdir)/src/mnb-launcher-button.c \
	$(top_srcdir)/src/mnb-launcher-application.c \
	$(top_srcdir)/src/mnb-launcher-grid.c \
	$(top_srcdir)/src/mnb-launcher-tree.c \
	$(top_srcdir)/src/moblin-netbook-launcher.c \
	test-launcher.c

test_launcher_button_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-button.c \
	test-launcher-button.c

test_entry_SOURCES = \
	$(top_srcdir)/src/mnb-entry.c \
	test-entry.c

test_launcher_monitor_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-application.c \
	$(top_srcdir)/src/mnb-launcher-tree.c \
	test-launcher-monitor.c

test_launcher_tree_SOURCES = \
	$(top_srcdir)/src/mnb-launcher-application.c \
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

test_panel_gtk_SOURCES = 	\
	test-panel.css		\
	test-panel-gtk.c

test_panel_gtk_LDADD = \
	$(MUTTER_PLUGIN_LIBS) \
	$(top_builddir)/libmnb/libmnbgtk.la

test_panel_clutter_SOURCES = 	\
	test-panel.css		\
	test-panel-clutter.c

test_panel_clutter_LDADD = \
	$(MUTTER_PLUGIN_LIBS) \
	$(top_builddir)/libmnb/libmnbclutter.la

CLEANFILES = test-panel-gtk.service test-panel-clutter.service \
	     test-panel.css

# dont want this installed, just generated
BUILT_SOURCES = test-panel-gtk.service test-panel-clutter.service \
		test-panel.css

EXTRA_DIST = $(service_in_files)

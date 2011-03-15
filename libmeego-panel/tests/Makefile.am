
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CPPFLAGS = \
	$(LIBMPL_CFLAGS) \
	-I$(top_srcdir)/libmeego-panel \
	-DDATADIR=\"$(datadir)\" \
  -DCSS_DIR=\"$(abs_builddir)\" \
	-DTHEMEDIR=\"$(MUTTER_MEEGO_THEME_DIR)/meego-panel\" \
	-DMEEGO_APP_LAUNCHES_STORE=\"$(libexecdir)/meego-app-launches-store\"

LDADD = $(LIBMPL_LIBS)

noinst_PROGRAMS = \
  test-content-pane \
	test-entry \
	test-icon-theme \
	test-panel-clutter \
	test-panel-gtk

test_entry_CFLAGS = \
	-DMX_CACHE=\"$(MUTTER_MEEGO_THEME_DIR)/mx.cache\" \
	-DTHEMEDIR=\"$(MUTTER_MEEGO_THEME_DIR)/meego-panel\"

# FIXME use this once split out
# -DTHEMEDIR=\"$(MUTTER_MEEGO_THEME_DIR)/$(PACKAGE_NAME)\"

test_content_pane_SOURCES = \
	$(top_srcdir)/libmeego-panel/meego-panel/mpl-content-pane.c \
	test-content-pane.c

test_entry_SOURCES = \
	$(top_srcdir)/libmeego-panel/meego-panel/mpl-entry.c \
	test-entry.c

test_icon_theme_SOURCES = \
	$(top_srcdir)/libmeego-panel/meego-panel/mpl-icon-theme.c \
	test-icon-theme.c

test_panel_clutter_DEPENDENCIES = \
	test-panel-clutter.service \
	test-panel.css

test_panel_clutter_LDADD = \
	$(LIBMPL_LIBS) \
	../meego-panel/libmeego-panel.la

test_panel_clutter_SOURCES = \
	test-panel-clutter.c

test_panel_gtk_DEPENDENCIES = \
	test-panel-gtk.service \
	test-panel.css

test_panel_gtk_LDADD = \
	$(LIBMPL_LIBS) \
	../meego-panel/libmeego-panel.la

test_panel_gtk_SOURCES = \
	test-panel-gtk.c

EXTRA_DIST = \
	test-panel-clutter.service.in \
	test-panel-gtk.service.in \
	test-panel.css.in

CLEANFILES = \
	test-panel-clutter.service \
	test-panel-gtk.service \
	test-panel.css

%.service: %.service.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@dir\@|$(abs_builddir)|" $< > $@

%.css: %.css.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@pkgdatadir\@|$(pkgdatadir)|" $< > $@


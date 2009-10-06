
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CPPFLAGS = \
	$(LIBMPL_CFLAGS) \
	-I$(top_srcdir)/libmoblin-panel \
	-DDATADIR=\"$(datadir)\" \
  -DCSS_DIR=\"$(abs_builddir)\"

LDADD = $(LIBMPL_LIBS)

noinst_PROGRAMS = \
	test-entry \
	test-icon-theme \
	test-panel-clutter \
	test-panel-gtk

test_entry_CFLAGS = \
	-DNBTK_CACHE=\"$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache\" \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/moblin-panel\"

# FIXME use this once split out
# -DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)\"

test_entry_SOURCES = \
	$(top_srcdir)/libmoblin-panel/moblin-panel/mpl-entry.c \
	test-entry.c

test_icon_theme_SOURCES = \
	$(top_srcdir)/libmoblin-panel/moblin-panel/mpl-icon-theme.c \
	test-icon-theme.c

test_panel_clutter_DEPENDENCIES = \
	test-panel-clutter.service \
	test-panel.css

test_panel_clutter_LDADD = \
	../moblin-panel/libmoblin-panel.la

test_panel_clutter_SOURCES = \
	test-panel-clutter.c

test_panel_gtk_DEPENDENCIES = \
	test-panel-gtk.service \
	test-panel.css

test_panel_gtk_LDADD = \
	../moblin-panel/libmoblin-panel.la

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


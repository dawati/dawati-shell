
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CPPFLAGS = \
	-I$(top_srcdir)/libmoblin-panel \
  -DCSS_DIR=\"$(abs_builddir)\"

noinst_PROGRAMS = \
	test-entry \
	test-panel-clutter \
	test-panel-gtk

test_entry_CFLAGS = \
	$(LIBMPL_CLUTTER_CFLAGS) \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/moblin-panel\"

# FIXME use this once split out
# -DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)\"

test_entry_LDADD = $(LIBMPL_CLUTTER_LIBS)

test_entry_SOURCES = \
	$(top_srcdir)/libmoblin-panel/moblin-panel/mpl-clutter-theme.c \
	$(top_srcdir)/libmoblin-panel/moblin-panel/mpl-entry.c \
	test-entry.c

test_panel_clutter_DEPENDENCIES = \
	test-panel-clutter.service \
	test-panel.css

test_panel_clutter_CFLAGS = $(LIBMPL_CLUTTER_CFLAGS)

test_panel_clutter_LDADD = \
	$(LIBMPL_CLUTTER_LIBS) \
	../moblin-panel/libmoblin-panel-clutter.la

test_panel_clutter_SOURCES = \
	test-panel-clutter.c

test_panel_gtk_DEPENDENCIES = \
	test-panel-gtk.service \
	test-panel.css

test_panel_gtk_CFLAGS = $(LIBMPL_GTK_CFLAGS)

test_panel_gtk_LDADD = \
	$(LIBMPL_GTK_LIBS) \
	../moblin-panel/libmoblin-panel-gtk.la

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


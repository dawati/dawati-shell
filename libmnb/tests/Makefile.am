
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

AM_CPPFLAGS = \
	-I$(top_srcdir)/libmnb \
  -DCSS_DIR=\"$(abs_builddir)\"

noinst_PROGRAMS = \
	test-entry \
	test-panel-clutter \
	test-panel-gtk

test_entry_CFLAGS = $(LIBMNB_CLUTTER_CFLAGS)

test_entry_LDADD = $(LIBMNB_CLUTTER_LIBS)

test_entry_SOURCES = \
	$(top_srcdir)/src/mnb-entry.c \
	test-entry.c

test_panel_clutter_DEPENDENCIES = \
	test-panel-clutter.service \
	test-panel.css

test_panel_clutter_CFLAGS = $(LIBMNB_CLUTTER_CFLAGS)

test_panel_clutter_LDADD = \
	$(LIBMNB_CLUTTER_LIBS) \
	../mnb/libmnb-clutter.la

test_panel_clutter_SOURCES = \
	test-panel-clutter.c

test_panel_gtk_DEPENDENCIES = \
	test-panel-gtk.service \
	test-panel.css

test_panel_gtk_CFLAGS = $(LIBMNB_GTK_CFLAGS)

test_panel_gtk_LDADD = \
	$(LIBMNB_GTK_LIBS) \
	../mnb/libmnb-gtk.la

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


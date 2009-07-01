
QUIET_GEN = $(Q:@=@echo '  GEN   '$@;)

SUBDIRS = moblin-panel theme tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	moblin-panel-gtk.pc \
	moblin-panel-clutter.pc

CLEANFILES = \
	moblin-panel-clutter.pc

EXTRA_DIST = \
	moblin-panel-gtk.pc.in \
	moblin-panel-clutter.pc.in.in

moblin-panel-clutter.pc: moblin-panel-clutter.pc.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@moblin_panel_clutter_theme_dir\@|$(MUTTER_MOBLIN_THEME_DIR)|g" < $< > $@


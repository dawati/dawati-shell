themedir = $(THEME_DIR)
theme_DATA = network-applet.css

%.css: %.css.in $(top_builddir)/config.log
	$(AM_V_GEN)sed -e "s|\@pkgdatadir\@|$(pkgdatadir)|" $< > $@

EXTRA_DIST = network-applet.css.in
DISTCLEANFILES = network-applet.css

-include $(top_srcdir)/git.mk

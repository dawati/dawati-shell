themedir = $(pkgdatadir)/theme

theme_DATA = power-applet.css volume-applet.css

%.css: %.css.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@pkgdatadir\@|$(pkgdatadir)|" $< > $@

EXTRA_DIST = power-applet.css.in volume-applet.css.in

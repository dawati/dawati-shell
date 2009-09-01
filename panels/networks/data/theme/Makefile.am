themedir = $(pkgdatadir)/theme

theme_DATA = network-applet.css

%.css: %.css.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@pkgdatadir\@|$(pkgdatadir)|" $< > $@

EXTRA_DIST = network-applet.css.in
DISTCLEANFILES = network-applet.css


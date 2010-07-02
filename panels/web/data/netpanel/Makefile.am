
themedir = $(pkgdatadir)/netpanel
dist_theme_DATA = \
	panel.css \
	ac-list.png \
	fallback-page.png \
	mpl-entry-bg.png \
	mpl-entry-clear-hover.png \
	mpl-entry-clear.png \
	mpl-entry-separator.png \
	newtab-thumbnail.png \
	placeholder-bg.png \
	thumbnail_hover.png \
	thumbnail.png \
	content-pane-background.png \
	content-pane-header.png \
	o2_globe.png \
	o2_history.png \
	o2_more.png \
	o2_search.png


# Manage mutter-meego's texture cache.
if ENABLE_CACHE
texture_cache = $(DESTDIR)$(pkgdatadir)/mx.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
endif


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
	thumbnail.png

# Manage mutter-moblin's texture cache.
if ENABLE_CACHE
texture_cache = $(DESTDIR)$(pkgdatadir)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
endif

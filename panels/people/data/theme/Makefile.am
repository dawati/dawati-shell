themedir = $(pkgdatadir)/theme
dist_theme_DATA = \
	panel.css \
	avatar-bg.png \
	grey-backdrop.png \
	people-search-bg.png \
	scroll-view-bin-bg.png \
	selected-highlight.png \
	selection-border.png \
	selection-empty.png \
	selection-full.png \
	tile-bg-hover.png \
	active-conversations-header.png

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

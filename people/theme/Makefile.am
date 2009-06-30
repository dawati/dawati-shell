
# FIXME use this once split out
# themedir = $(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)
themedir = $(MUTTER_MOBLIN_THEME_DIR)/moblin-panel-people
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
	tile-bg-hover.png

# Manage mutter-moblin's texture cache.

texture_cache = $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)

uninstall-hook:
	rm -f $(texture_cache)


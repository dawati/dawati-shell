
# FIXME use this once split out
# themedir = $(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)
themedir = $(MUTTER_MOBLIN_THEME_DIR)/moblin-panel-pasteboard
dist_theme_DATA = \
	panel.css \
	pasteboard-delete-bg.png \
	pasteboard-delete-hover.png \
	pasteboard-delete.png \
	pasteboard-item-delete-hover.png \
	pasteboard-item-delete.png \
	pasteboard-item-hover.png \
	pasteboard-items-list-bg.png \
	pasteboard-search-bg.png \
	placeholder-bg.png \
	search-entry-bg.png \
	search-entry-clear-hover.png \
	search-entry-clear.png \
	search-entry-separator.png

# Manage mutter-moblin's texture cache.

texture_cache = $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)

uninstall-hook:
	rm -f $(texture_cache)


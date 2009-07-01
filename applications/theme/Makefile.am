
# FIXME use this once split out
# themedir = $(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)
themedir = $(MUTTER_MOBLIN_THEME_DIR)/moblin-panel-applications
dist_theme_DATA = \
	toolbar-button.css \
	apps-coloured.png \
	apps-normal.png \
	\
	panel.css \
	launcher-button-fav-toggle-active.png \
	launcher-button-fav-toggle-checked.png \
	launcher-button-fav-toggle-hover.png \
	launcher-button-fav-toggle.png \
	launcher-hover.png \
	launcher-search-bg.png

# Manage mutter-moblin's texture cache.

if ENABLE_CACHE
texture_cache = $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)
endif

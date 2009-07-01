
# FIXME use this once split out
# themedir = $(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)
themedir = $(MUTTER_MOBLIN_THEME_DIR)/moblin-panel-status
dist_theme_DATA = \
	panel.css \
	decoration-frame.png \
	edit_close_hover.png \
	edit_close.png \
	entry-frame-edit.png \
	entry-frame-hover.png \
	im-available.png \
	im-away.png \
	im-busy.png \
	im-offline.png \
	no_image_icon.png \
	placeholder-bg.png \
	plus-button-checked-hover.png \
	plus-button-checked.png \
	plus-button-hover.png \
	plus-button.png

# Manage mutter-moblin's texture cache.

texture_cache = $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)


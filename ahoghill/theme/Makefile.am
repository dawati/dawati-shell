
# FIXME use this once split out
# themedir = $(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)
themedir = $(MUTTER_MOBLIN_THEME_DIR)/moblin-panel-media
dist_theme_DATA = \
	panel.css \
	bot_playque_closed.png \
	bot_playque_hover.png \
	hrn-generic-album.png \
	hrn-generic-images.png \
	hrn-generic-video.png \
	hrn-progress-bar-handle.png \
	hrn-progress-bar-through.png \
	media_search_bkgrd.png \
	mid_playque_closed.png \
	mid_playque_hover.png \
	mid_playque_selected.png \
	placeholder-bg.png \
	play_hover.png \
	playque_background.png \
	playqueue-placeholder.png \
	pq_pause_hover.png \
	pq_pause.png \
	pq_play_hover.png \
	pq_play.png \
	search_field_crop.png \
	thumbnail_hover.png \
	thumbnail.png \
	top_playque_closed.png \
	top_playque_hover.png \
	top_playque_selected.png

# Manage mutter-moblin's texture cache.

texture_cache = $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)

uninstall-hook:
	rm -f $(texture_cache)


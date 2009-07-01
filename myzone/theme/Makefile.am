
# FIXME use this once split out
# themedir = $(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)
themedir = $(MUTTER_MOBLIN_THEME_DIR)/moblin-panel-myzone
dist_theme_DATA = \
	panel.css \
	backdrop.png \
	calendar-icon.png \
	check_box.png \
	checked_box.png \
	date-background-background.png \
	date-background.png \
	default-album-artwork.png \
	default-avatar-icon.png \
	double-div-line.png \
	event-background-dark.png \
	general-background-dark.png \
	general-background.png \
	linus.png \
	no-content-tile-bg.png \
	no-content-tile-glow-bg.png \
	people-background.png \
	quotebackground.png \
	recent-files-background.png \
	single-div-line.png \
	tile-bg.png \
	tile-glow.png \
	tile-partial-glow.png \
	time-bg-past.png \
	time-bg.png \
	toys.jpg \
	v-div-line.png

# Manage mutter-moblin's texture cache.

texture_cache = $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)

uninstall-hook:
	rm -f $(texture_cache)


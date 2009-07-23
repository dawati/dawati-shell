
# FIXME use this once split out
# themedir = $(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)
themedir = $(MUTTER_MOBLIN_THEME_DIR)/moblin-panel-internet
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
	vscroll-backward-hover.png \
	vscroll-backward.png \
	vscroll-forward-hover.png \
	vscroll-forward.png \
	vscroll-handle-hover.png \
	vscroll-handle.png

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

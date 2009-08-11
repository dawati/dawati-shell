# Use a separate nbtk.cache for the moblin-panel library generic assets so we
# don't need to load in all mutter-moblin assets
themedir = $(MUTTER_MOBLIN_THEME_DIR)/moblin-panel
dist_theme_DATA = \
	theme.css \
	mpl-entry-bg.png \
	mpl-entry-clear-hover.png \
	mpl-entry-clear.png \
	mpl-entry-separator.png

if ENABLE_CACHE
texture_cache = $(DESTDIR)$(themedir)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(themedir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(themedir)
endif

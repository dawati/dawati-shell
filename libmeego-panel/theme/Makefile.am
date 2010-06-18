# Use a separate mx.cache for the meego-panel library generic assets so we
# don't need to load in all mutter-meego assets
themedir = $(MUTTER_MEEGO_THEME_DIR)/meego-panel
dist_theme_DATA = \
	theme.css \
	content-pane-background.png \
  content-pane-header.png \
	mpl-entry-bg.png \
	mpl-entry-clear-hover.png \
	mpl-entry-clear.png \
	mpl-entry-separator.png

if ENABLE_CACHE
texture_cache = $(DESTDIR)$(themedir)/mx.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(themedir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(themedir)
endif

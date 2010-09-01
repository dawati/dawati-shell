themedir = $(pkgdatadir)/theme
dist_theme_DATA = \
	panel.css \
	avatar-bg.png \
	avatar_icon.png \
	avatar_frame.png \
	grey-backdrop.png \
	people-search-bg.png \
	scroll-view-bin-bg.png \
	selected-highlight.png \
	selection-border.png \
	selection-empty.png \
	selection-full.png \
	tile-bg-hover.png \
	active-conversations-header.png \
	active-content-table-bg.png \
	unread-message-bubble-bg.png \
	box-background.png \
	frame-header-background.png \
	frame-background.png \
	panel-background.png \
	content-panel-background.png \
	content-panel-header-grey.png \
	box-background-border-bottom.png \
	background-gradient.png \
	people.png

# Manage mutter-meego's texture cache.
if ENABLE_CACHE
texture_cache = $(DESTDIR)$(pkgdatadir)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
endif

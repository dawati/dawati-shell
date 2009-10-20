themedir = $(pkgdatadir)/theme
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
	minus-button-hover.png \
	minus-button.png \
	plus-button-hover.png \
	plus-button.png \
	avatar-bg.png \
	launcher-button-bg.png

# Manage mutter-moblin's texture cache.

if ENABLE_CACHE
texture_cache = $(DESTDIR)$(pkgdatadir)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
endif

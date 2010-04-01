themedir = $(pkgdatadir)/style
dist_theme_DATA = \
	content-panel-background.png \
	content-panel-header-grey.png \
	content-panel-header-orange.png \
	new-zone-title-active.png \
	new-zone-title-focus.png \
	new-zone-title-hover.png \
	switcher.css \
	thumbnail-active.png \
	thumbnail-focused.png \
	thumbnail-hover.png \
	thumbnail-target.png \
	workspace-title-new-symbol-active.png \
	zone-bg-active.png \
	zone-bg-focus.png \
	zone-bg-hover.png \
	zone-title-focused.png \
	zone-title-hover.png \
	zone-title-normal.png \
	close-button.png \
	close-button-hover.png

# Manage mutter-moblin's texture cache.
#if ENABLE_CACHE
#texture_cache = $(DESTDIR)$(pkgdatadir)/nbtk.cache
#CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

#install-data-hook:
#	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
#
#uninstall-hook:
#	rm -f $(texture_cache)
#	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
#endif

-include $(top_srcdir)/git.mk

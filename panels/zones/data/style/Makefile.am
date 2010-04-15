themedir = $(pkgdatadir)/style
dist_theme_DATA = \
	app-background-panel.png \
	app-background-panel-highlight.png \
	content-panel-background.png \
	content-panel-background-highlight-orange.png \
	content-panel-header-grey.png \
	content-panel-header-orange.png \
	switcher.css \
	workspace-title-new-symbol-active.png \
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

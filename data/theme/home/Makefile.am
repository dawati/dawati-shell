themedir = $(PANEL_HOME_THEMEDIR)
dist_theme_DATA = \
	panel.css \
	selector.png \
	selector-empty.png \
	$(NULL)

# Manage mutter-dawati's texture cache.
if ENABLE_CACHE
texture_cache = $(DESTDIR)$(pkgdatadir)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
endif

include $(top_srcdir)/build/autotools/Makefile.am.panels

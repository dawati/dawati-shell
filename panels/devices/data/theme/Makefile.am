
themedir = $(pkgdatadir)/theme
dist_theme_DATA = \
  toolbar-button.css \
  content-pane-background.png \
  content-pane-header.png \
  devices-coloured.png \
  devices-normal.png \
  directory-highlight.png \
  frame-border.png \
  panel.css \
  panel-background.png \
  seperator.png \
  volume-bars-0.png \
  $(NULL)

# Manage mutter-moblin's texture cache.

if ENABLE_CACHE
texture_cache = $(DESTDIR)$(pkgdatadir)/mx.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
endif


-include $(top_srcdir)/git.mk

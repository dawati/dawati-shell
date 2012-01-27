SUBDIRS = icons

themedir = $(PANEL_DEVICES_THEMEDIR)
dist_theme_DATA = \
	toolbar-button.css \
	content-pane-header.png \
	devices-coloured.png \
	devices-normal.png \
	directory-highlight.png \
	frame-border.png \
	panel.css \
	seperator.png \
	volume-bars-0.png \
	icon-slider-brightness-on.png \
	icon-slider-brightness-off.png \
	icon-slider-volume-on.png \
	icon-slider-volume-off.png \
	$(NULL)

# Manage mutter-dawati's texture cache.

if ENABLE_CACHE
texture_cache = $(DESTDIR)$(pkgdatadir)/mx.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
endif

include $(top_srcdir)/build/autotools/Makefile.am.panels

-include $(top_srcdir)/git.mk

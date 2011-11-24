SUBDIRS = \
	applications \
	bluetooth \
	datetime \
	devices \
	myzone \
	networks \
	people \
	power-icon \
	status \
	web \
	zones \
	$(NULL)

themedir = $(DAWATI_THEME_DIR)
dist_theme_DATA = \
	mutter-dawati.css \
	$(NULL)

theme_paneldir = $(DAWATI_THEME_DIR)/panel
dist_theme_panel_DATA = \
	panel/apps-coloured.png \
	panel/apps-normal.png \
	panel/clock-background.png \
	panel/devices-coloured.png \
	panel/devices-normal.png \
	panel/devices-active-coloured.png \
	panel/devices-active-normal.png \
	panel/internet-coloured.png \
	panel/internet-normal.png \
	panel/media-coloured.png \
	panel/media-normal.png \
	panel/myzone-coloured.png \
	panel/myzone-normal.png \
	panel/network-coloured.png \
	panel/network-normal.png \
	panel/panel-background.png \
	panel/panel-shadow.png \
	panel/pasteboard-coloured.png \
	panel/pasteboard-normal.png \
	panel/people-coloured.png \
	panel/people-normal.png \
	panel/people-active-coloured.png \
	panel/people-active-normal.png \
	panel/zones-coloured.png \
	panel/zones-normal.png \
	panel/zones-1-coloured.png \
	panel/zones-1-normal.png \
	panel/zones-2-coloured.png \
	panel/zones-2-normal.png \
	panel/zones-3-coloured.png \
	panel/zones-3-normal.png \
	panel/zones-4-coloured.png \
	panel/zones-4-normal.png \
	panel/status-coloured.png \
	panel/status-normal.png \
	panel/toolbar-button-normal.png \
	panel/toolbar-button-active.png \
	panel/toolbar-button-hover.png \
	panel/toolbar-button-tray.png \
	$(NULL)

theme_genericdir = $(DAWATI_THEME_DIR)/generic
dist_theme_generic_DATA = \
	generic/checkbox.png \
	generic/close-button.png \
	generic/close-button-hover.png \
	generic/spinner.png \
	generic/search-entry-bg.png \
	generic/search-entry-clear-hover.png \
	generic/search-entry-clear.png \
	generic/search-entry-separator.png \
	generic/tooltip-arrow.png \
	generic/tooltip-background.png \
	generic/vscroll-backward-hover.png \
	generic/vscroll-backward.png \
	generic/vscroll-forward-hover.png \
	generic/vscroll-forward.png \
	generic/vscroll-handle-hover.png \
	generic/vscroll-handle.png \
	generic/placeholder-bg.png \
	$(NULL)

theme_alttabdir = $(DAWATI_THEME_DIR)/alttab
dist_theme_alttab_DATA = \
	alttab/application-normal.png \
	alttab/application-active.png \
	$(NULL)

theme_dropdowndir = $(DAWATI_THEME_DIR)/drop-down
dist_theme_dropdown_DATA = \
	drop-down/panel_shadow.png \
	drop-down/panel_stub_shadow.png \
	$(NULL)

theme_cappletdir = $(DAWATI_THEME_DIR)/capplet
dist_theme_capplet_DATA = \
	capplet/capplet-background.png \
	capplet/close-icon.png \
	capplet/close-icon-hover.png \
	capplet/content-background.png \
	capplet/content-header.png \
	capplet/drop-target-area.png \
	capplet/toolbar-background.png \
	capplet/button-background.png \
	$(NULL)

theme_notifiersdir = $(DAWATI_THEME_DIR)/notifiers
dist_theme_notifiers_DATA = \
	notifiers/notification-bg.png \
	notifiers/fscreen-notif-hover.png \
	notifiers/fscreen-notif-normal.png \
	$(NULL)

theme_effectsdir = $(DAWATI_THEME_DIR)/effects
dist_theme_effects_DATA = \
	effects/zone-switch-background.png \
	effects/bin-border-image.png \
	$(NULL)

# Manage mutter-dawati's texture cache.
if ENABLE_CACHE
texture_cache = $(DESTDIR)$(DAWATI_THEME_DIR)/mx.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(DAWATI_THEME_DIR)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(DAWATI_THEME_DIR)
endif

include $(top_srcdir)/build/autotools/Makefile.am.panels

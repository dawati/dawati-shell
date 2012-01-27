SUBDIRS = \
	applications \
	bluetooth \
	common \
	datetime \
	devices \
	music \
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
	panel/icon-apps-highlight.png \
	panel/icon-apps.png \
	panel/clock-background.png \
	panel/icon-devices-highlight.png \
	panel/icon-devices.png \
	panel/icon-devices-active-highlight.png \
	panel/icon-devices-active.png \
	panel/icon-internet-highlight.png \
	panel/icon-internet.png \
	panel/icon-music-highlight.png \
	panel/icon-music.png \
	panel/icon-myzone-highlight.png \
	panel/icon-myzone.png \
	panel/icon-network-highlight.png \
	panel/icon-network.png \
	panel/toolbar-bg.png \
	panel/panel-shadow.png \
	panel/icon-pasteboard-highlight.png \
	panel/icon-pasteboard.png \
	panel/icon-im-highlight.png \
	panel/icon-im.png \
	panel/icon-im-active-highlight.png \
	panel/icon-im-active.png \
	panel/icon-switcher-highlight.png \
	panel/icon-switcher.png \
	panel/icon-switcher-highlight-1.png \
	panel/icon-switcher-1.png \
	panel/icon-switcher-highlight-2.png \
	panel/icon-switcher-2.png \
	panel/icon-switcher-highlight-3.png \
	panel/icon-switcher-3.png \
	panel/icon-switcher-highlight-4.png \
	panel/icon-switcher-4.png \
	panel/icon-status-highlight.png \
	panel/icon-status.png \
	panel/toolbar-button-normal.png \
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

theme_cappletdir = $(DAWATI_THEME_DIR)/capplet
dist_theme_capplet_DATA = \
	capplet/capplet-background.png \
	capplet/close-icon.png \
	capplet/close-icon-hover.png \
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

SUBDIRS = \
	applications \
	bluetooth \
	common \
	datetime \
	devices \
	music \
	myzone \
	networks \
	panel \
	people \
	power-icon \
	shared \
	status \
	switcher \
	web \
	zones \
	$(NULL)

themedir = $(DAWATI_THEME_DIR)
dist_theme_DATA = \
	mutter-dawati.css \
	$(NULL)

theme_paneldir = $(DAWATI_THEME_DIR)/toolbar
dist_theme_panel_DATA = \
	toolbar/icon-apps-highlight.png \
	toolbar/icon-apps.png \
	toolbar/icon-battery-20.png \
	toolbar/icon-battery-40.png \
	toolbar/icon-battery-60.png \
	toolbar/icon-battery-80.png \
	toolbar/icon-battery-100.png \
	toolbar/icon-battery-empty.png \
	toolbar/icon-battery-missing.png \
	toolbar/icon-battery-plugged.png \
	toolbar/icon-battery-20-highlight.png \
	toolbar/icon-battery-40-highlight.png \
	toolbar/icon-battery-60-highlight.png \
	toolbar/icon-battery-80-highlight.png \
	toolbar/icon-battery-100-highlight.png \
	toolbar/icon-battery-empty-highlight.png \
	toolbar/icon-battery-missing-highlight.png \
	toolbar/icon-battery-plugged-highlight.png \
	toolbar/icon-bluetooth-connecting-highlight.png \
	toolbar/icon-bluetooth-connecting.png \
	toolbar/icon-bluetooth-idle-highlight.png \
	toolbar/icon-bluetooth-idle.png \
	toolbar/icon-bluetooth-off-highlight.png \
	toolbar/icon-bluetooth-off.png \
	toolbar/icon-devices-highlight.png \
	toolbar/icon-devices.png \
	toolbar/icon-devices-active-highlight.png \
	toolbar/icon-devices-active.png \
	toolbar/icon-internet-highlight.png \
	toolbar/icon-internet.png \
	toolbar/icon-music-highlight.png \
	toolbar/icon-music.png \
	toolbar/icon-myzone-highlight.png \
	toolbar/icon-myzone.png \
	toolbar/icon-myzone-tiny.png \
	toolbar/icon-network-highlight.png \
	toolbar/icon-network.png \
	toolbar/toolbar-bg.png \
	toolbar/toolbar-shadow.png \
	toolbar/icon-pasteboard-highlight.png \
	toolbar/icon-pasteboard.png \
	toolbar/icon-im-highlight.png \
	toolbar/icon-im.png \
	toolbar/icon-im-active-highlight.png \
	toolbar/icon-im-active.png \
	toolbar/icon-switcher-highlight.png \
	toolbar/icon-switcher.png \
	toolbar/icon-switcher-highlight-1.png \
	toolbar/icon-switcher-1.png \
	toolbar/icon-switcher-highlight-2.png \
	toolbar/icon-switcher-2.png \
	toolbar/icon-switcher-highlight-3.png \
	toolbar/icon-switcher-3.png \
	toolbar/icon-switcher-highlight-4.png \
	toolbar/icon-switcher-4.png \
	toolbar/icon-status-highlight.png \
	toolbar/icon-status.png \
	toolbar/toolbar-button-normal.png \
	toolbar/toolbar-button-hover.png \
	toolbar/toolbar-button-tray.png \
	toolbar/toolbar-selection.png \
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

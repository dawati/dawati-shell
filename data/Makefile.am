schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = mutter-meego.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA)
else
install-data-local:
endif

themedir = $(MUTTER_MEEGO_THEME_DIR)
dist_theme_DATA = \
	theme/mutter-meego.css \
	theme/LICENSE

theme_paneldir = $(MUTTER_MEEGO_THEME_DIR)/panel
dist_theme_panel_DATA = \
	theme/panel/apps-coloured.png \
	theme/panel/apps-normal.png \
	theme/panel/clock-background.png \
	theme/panel/devices-coloured.png \
	theme/panel/devices-normal.png \
	theme/panel/devices-active-coloured.png \
	theme/panel/devices-active-normal.png \
	theme/panel/internet-coloured.png \
	theme/panel/internet-normal.png \
	theme/panel/media-coloured.png \
	theme/panel/media-normal.png \
	theme/panel/myzone-coloured.png \
	theme/panel/myzone-normal.png \
	theme/panel/network-coloured.png \
	theme/panel/network-normal.png \
	theme/panel/panel-background.png \
	theme/panel/panel-shadow.png \
	theme/panel/pasteboard-coloured.png \
	theme/panel/pasteboard-normal.png \
	theme/panel/people-coloured.png \
	theme/panel/people-normal.png \
	theme/panel/people-active-coloured.png \
	theme/panel/people-active-normal.png \
	theme/panel/spaces-coloured.png \
	theme/panel/spaces-normal.png \
	theme/panel/status-coloured.png \
	theme/panel/status-normal.png \
	theme/panel/toolbar-button-normal.png \
	theme/panel/toolbar-button-active.png \
	theme/panel/toolbar-button-hover.png \
	theme/panel/toolbar-button-tray.png

theme_genericdir = $(MUTTER_MEEGO_THEME_DIR)/generic
dist_theme_generic_DATA = \
	theme/generic/checkbox.png	\
	theme/generic/close-button.png	\
	theme/generic/close-button-hover.png	\
	theme/generic/spinner.png	\
	theme/generic/search-entry-bg.png \
	theme/generic/search-entry-clear-hover.png \
	theme/generic/search-entry-clear.png \
	theme/generic/search-entry-separator.png \
	theme/generic/tooltip-arrow.png \
	theme/generic/tooltip-background.png \
	theme/generic/vscroll-backward-hover.png \
	theme/generic/vscroll-backward.png \
	theme/generic/vscroll-forward-hover.png \
	theme/generic/vscroll-forward.png \
	theme/generic/vscroll-handle-hover.png \
	theme/generic/vscroll-handle.png \
	theme/generic/placeholder-bg.png

theme_alttabdir = $(MUTTER_MEEGO_THEME_DIR)/alttab
dist_theme_alttab_DATA = \
	theme/alttab/application-normal.png \
	theme/alttab/application-active.png

theme_dropdowndir = $(MUTTER_MEEGO_THEME_DIR)/drop-down
dist_theme_dropdown_DATA = \
	theme/drop-down/panel_shadow.png	\
	theme/drop-down/panel_stub_shadow.png

theme_cappletdir = $(MUTTER_MEEGO_THEME_DIR)/capplet
dist_theme_capplet_DATA = \
	theme/capplet/capplet-background.png	\
	theme/capplet/close-icon.png		\
	theme/capplet/close-icon-hover.png	\
	theme/capplet/content-background.png	\
	theme/capplet/content-header.png	\
	theme/capplet/drop-target-area.png	\
	theme/capplet/toolbar-background.png	\
	theme/capplet/button-background.png

theme_notifiersdir = $(MUTTER_MEEGO_THEME_DIR)/notifiers
dist_theme_notifiers_DATA = \
	theme/notifiers/notification-bg.png	\
	theme/notifiers/fscreen-notif-hover.png \
	theme/notifiers/fscreen-notif-normal.png

theme_effectsdir = $(MUTTER_MEEGO_THEME_DIR)/effects
dist_theme_effects_DATA = \
	theme/effects/zone-switch-background.png \
	theme/effects/bin-border-image.png

# Manage mutter-meego's texture cache.
if ENABLE_CACHE
texture_cache = $(DESTDIR)$(MUTTER_MEEGO_THEME_DIR)/mx.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MEEGO_THEME_DIR)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MEEGO_THEME_DIR)
endif

CLEANFILES = $(schema_DATA)
EXTRA_DIST = $(schema_in_files)

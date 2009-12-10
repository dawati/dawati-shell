@INTLTOOL_DESKTOP_RULE@

desktopfilesdir=$(pkgdatadir)/panels
desktopfiles_in_files=moblin-panel-zones.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)
desktopfiles_DATA = $(desktopfiles_files)

themedir = $(MUTTER_MOBLIN_THEME_DIR)
dist_theme_DATA = \
	theme/mutter-moblin.css

theme_paneldir = $(MUTTER_MOBLIN_THEME_DIR)/panel
dist_theme_panel_DATA = \
	theme/panel/apps-coloured.png \
	theme/panel/apps-normal.png \
	theme/panel/bluetooth-coloured.png \
	theme/panel/bluetooth-normal.png \
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
	theme/panel/background-tile.png \
	theme/panel/toolbar-button-active.png \
	theme/panel/toolbar-button-hover.png \
	theme/panel/toolbar-hint.png

theme_chooserdir = $(MUTTER_MOBLIN_THEME_DIR)/chooser
dist_theme_chooser_DATA = \
	theme/chooser/background.png		\
	theme/chooser/thumb-selected.png	\
	theme/chooser/thumb-unselected.png	\
	theme/chooser/space-selected.png	\
	theme/chooser/space-unselected.png

theme_genericdir = $(MUTTER_MOBLIN_THEME_DIR)/generic
dist_theme_generic_DATA = \
	theme/generic/checkbox.png	\
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

theme_switcherdir = $(MUTTER_MOBLIN_THEME_DIR)/switcher
dist_theme_switcher_DATA = \
	theme/switcher/application-active.png \
	theme/switcher/application-close-button.png \
	theme/switcher/application-close-button-hover.png \
	theme/switcher/application-inactive.png \
	theme/switcher/workspace-background-active.png \
	theme/switcher/workspace-background-new.png \
	theme/switcher/workspace-background-new-active.png \
	theme/switcher/workspace-background-new-over.png \
	theme/switcher/workspace-background-new-disabled.png \
	theme/switcher/workspace-background.png \
	theme/switcher/workspace-title-active.png \
	theme/switcher/workspace-title-new.png \
	theme/switcher/workspace-title-new-active.png \
	theme/switcher/workspace-title-new-over.png \
	theme/switcher/workspace-title-new-disabled.png \
	theme/switcher/workspace-title-new-symbol-active.png \
	theme/switcher/workspace-title-new-symbol-over.png \
	theme/switcher/workspace-title-new-symbol-disabled.png \
	theme/switcher/workspace-title.png

theme_dropdowndir = $(MUTTER_MOBLIN_THEME_DIR)/drop-down
dist_theme_dropdown_DATA = \
	theme/drop-down/panel_arrow.png \
	theme/drop-down/panel_arrow_hover.png \
	theme/drop-down/panel_up_button.png \
	theme/drop-down/panel_up_button_hover.png \
	theme/drop-down/panel_shadow.png

theme_notifiersdir = $(MUTTER_MOBLIN_THEME_DIR)/notifiers
dist_theme_notifiers_DATA = \
	theme/notifiers/notification-bg.png	\
	theme/notifiers/fscreen-notif-hover.png \
	theme/notifiers/fscreen-notif-normal.png

theme_effectsdir = $(MUTTER_MOBLIN_THEME_DIR)/effects
dist_theme_effects_DATA = \
	theme/effects/zone-switch-background.png

# Manage mutter-moblin's texture cache.
if ENABLE_CACHE
texture_cache = $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(MUTTER_MOBLIN_THEME_DIR)
endif

CLEANFILES = $(desktopfiles_files)
EXTRA_DIST = $(desktopfiles_in_files)

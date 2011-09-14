themedir = $(pkgdatadir)/theme
dist_theme_DATA = \
	date-panel.css \
	edit-clear.png \
	edit-find.png \
	dp-scroll-view-bin-bg.png \
	dismiss_button.png \
	dismiss_button_hover.png \
	alarm.png	\
	up.png		\
	down.png	\
	am-pm-toggle-background.png \
	snooze-background.png \
	single-div-line.png	\
	v-div-line.png	\
	double-div-line.png	\
	time-bg.png	\
	time-bg-past.png	\
	check_box.png	\
	checked_box.png	\
	backdrop.png	\
	panel-header-bar.png	\
	clock.png	\
	time-panel.png	\
	panel-header-widget-bar.png	\
	main-panel.png	\
	button.png	\
	clock-day.png	\
	clock-night.png \
	drop-down-panel.png 	\
	panel-background.png	\
	appointments-icon.png	\
	tasks-icon.png	\
	panel-header-widget-bar-border.png	\
	panel-header-widget-bar-border-up.png	\
	event-task-tile-bg.png


# Manage mutter-meego's texture cache.
if ENABLE_CACHE
texture_cache = $(DESTDIR)$(pkgdatadir)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
endif

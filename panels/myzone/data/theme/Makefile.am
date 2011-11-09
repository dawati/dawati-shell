themedir = $(pkgdatadir)/theme
dist_theme_DATA = \
	panel.css \
	backdrop.png \
	calendar-icon.png \
	check_box.png \
	checked_box.png \
	default-album-artwork.png \
	default-avatar-icon.png \
	double-div-line.png \
	linus.png \
	no-content-tile-bg.png \
	no-content-tile-glow-bg.png \
	quotebackground.png \
	single-div-line.png \
	tile-bg.png \
	tile-partial-glow.png \
	time-bg-past.png \
	time-bg.png \
	toys.jpg \
	v-div-line.png \
	calendar-icon-1.png \
	calendar-icon-2.png \
	calendar-icon-3.png \
	calendar-icon-4.png \
	calendar-icon-5.png \
	calendar-icon-6.png \
	calendar-icon-7.png \
	calendar-icon-8.png \
	calendar-icon-9.png \
	calendar-icon-10.png \
	calendar-icon-11.png \
	calendar-icon-12.png \
	calendar-icon-13.png \
	calendar-icon-14.png \
	calendar-icon-15.png \
	calendar-icon-16.png \
	calendar-icon-17.png \
	calendar-icon-18.png \
	calendar-icon-19.png \
	calendar-icon-20.png \
	calendar-icon-21.png \
	calendar-icon-22.png \
	calendar-icon-23.png \
	calendar-icon-24.png \
	calendar-icon-25.png \
	calendar-icon-26.png \
	calendar-icon-27.png \
	calendar-icon-28.png \
	calendar-icon-29.png \
	calendar-icon-30.png \
	calendar-icon-31.png \
	tile-dismiss-icon-hover.png \
	tile-dismiss-icon-normal.png \
	cat.png \
	avatar-bg.png \
	avatar_frame.png \
	avatar_icon.png \
	card_frame.png \
	card_frame_hover.png \
	dismiss_button.png \
	dismiss_button_hover.png \
	count-frame.png \
	count-frame-zero.png \
	frame-header.png \
	content-panel-background-highlight.png \
	content-panel-background.png \
	content-panel-header-grey.png \
	tasks-icon.png \
	top-fade.png \
	event-task-tile-bg.png \
	intro-video-thumbnail.png \
	play-button.png


if ENABLE_CACHE
texture_cache = $(DESTDIR)$(pkgdatadir)/nbtk.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(NBTK_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
endif

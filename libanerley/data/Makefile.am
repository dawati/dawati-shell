assetsdir = $(pkgdatadir)

assets_DATA = style.css \
	      avatar-bg.png \
	      default-avatar-image.png \
	      avatar_icon.png
	      tile-bg-hover.png \
	      presence-available.png \
	      presence-away.png \
	      presence-offline.png \
	      presence-busy.png \
	      avatar_frame.png

desktopfiles_in_files=anerley-account-starter.desktop.in
desktopfiles_files=$(desktopfiles_in_files:.desktop.in=.desktop)

%.desktop: %.desktop.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

autostartdir = $(sysconfdir)/xdg/autostart
autostart_DATA = $(desktopfiles_files)

EXTRA_DIST=$(assets_DATA) $(desktopfiles_in_files)



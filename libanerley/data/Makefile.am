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

%.desktop.in: %.desktop.in.in Makefile
	$(QUIET_GEN)sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST=$(assets_DATA) anerley-account-starter.desktop.in



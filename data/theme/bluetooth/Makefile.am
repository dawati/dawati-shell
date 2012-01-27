themedir = $(PANEL_BLUETOOTH_THEMEDIR)
theme_DATA = \
	bluetooth-panel.css \
	$(NULL)
dist_theme_DATA = \
	icon-bluetooth-off-highlight.png \
	icon-bluetooth-off.png \
	icon-bluetooth-idle-highlight.png \
	icon-bluetooth-idle.png \
	icon-bluetooth-connecting-highlight.png \
	icon-bluetooth-connecting.png \
	$(NULL)

EXTRA_DIST = bluetooth-panel.css.in
DISTCLEANFILES = $(theme_DATA)

include $(top_srcdir)/build/autotools/Makefile.am.panels

-include $(top_srcdir)/git.mk

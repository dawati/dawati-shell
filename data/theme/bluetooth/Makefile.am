themedir = $(PANEL_BLUETOOTH_THEMEDIR)
theme_DATA = \
	bluetooth-panel.css \
	$(NULL)
dist_theme_DATA = \
	bluetooth-off-hover.png \
	bluetooth-off.png \
	bluetooth-idle-hover.png \
	bluetooth-idle.png \
	bluetooth-connecting-hover.png \
	bluetooth-connecting.png \
	$(NULL)

EXTRA_DIST = bluetooth-panel.css.in
DISTCLEANFILES = $(theme_DATA)

include $(top_srcdir)/build/autotools/Makefile.am.panels

-include $(top_srcdir)/git.mk

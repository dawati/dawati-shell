
themedir = $(PANEL_POWER_ICON_THEMEDIR)
nodist_theme_DATA = power-icon.css

icondir = $(PANEL_POWER_ICON_THEMEDIR)/icons
dist_icon_DATA = \
	battery-20.png \
	battery-40.png \
	battery-60.png \
	battery-80.png \
	battery-empty.png \
	battery-full.png \
	battery-missing.png \
	battery-plugged.png \
	$(NULL)

EXTRA_DIST = \
	power-icon.css.in \
	$(NULL)

CLEANFILES = \
	power-icon.css \
	$(NULL)

include $(top_srcdir)/build/autotools/Makefile.am.panels

-include $(top_srcdir)/git.mk

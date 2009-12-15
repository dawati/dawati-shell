
noinst_LTLIBRARIES = libeffects.la

AM_CFLAGS = \
	@MUTTER_PLUGIN_CFLAGS@ \
	-DMX_CACHE=\"$(MUTTER_MOBLIN_THEME_DIR)/mx.cache\" \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)\"

libeffects_la_LIBADD = $(MUTTER_PLUGIN_LIBS)
libeffects_la_SOURCES =					\
			mnb-switch-zones-effect.c	\
			mnb-switch-zones-effect.h

libeffects_la_CPPFLAGS = $(effects_cflags)
libeffects_la_LDFLAGS = $(LDADD)


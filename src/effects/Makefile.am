
noinst_LTLIBRARIES = libeffects.la

AM_CFLAGS = \
	@MUTTER_PLUGIN_CFLAGS@ \
	-DMX_CACHE=\"$(DAWATI_THEME_DIR)/mx.cache\" \
	-DTHEMEDIR=\"$(DAWATI_RUNTIME_THEME_DIR)\" \
	-I$(top_srcdir)/src

libeffects_la_LIBADD = $(MUTTER_PLUGIN_LIBS)
libeffects_la_SOURCES =					\
			mnb-switch-zones-effect.c	\
			mnb-switch-zones-effect.h	\
			mnb-fancy-bin.c			\
			mnb-fancy-bin.h			\
			mnb-zones-preview.c		\
			mnb-zones-preview.h

libeffects_la_CPPFLAGS = $(effects_cflags)
libeffects_la_LDFLAGS = $(LDADD)


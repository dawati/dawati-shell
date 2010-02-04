
noinst_LTLIBRARIES = libalttab.la

AM_CFLAGS = \
	@MUTTER_PLUGIN_CFLAGS@ \
	-DMX_CACHE=\"$(MUTTER_MOBLIN_THEME_DIR)/mx.cache\" 	\
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)\"		\
	-I$(top_srcdir)/libmoblin-panel

libalttab_la_LIBADD = $(MUTTER_PLUGIN_LIBS)
libalttab_la_SOURCES =					\
			mnb-alttab.c			\
			mnb-alttab.h			\
			mnb-alttab-keys.c		\
			mnb-alttab-keys.h		\
			mnb-alttab-overlay.c		\
			mnb-alttab-overlay.h		\
			mnb-alttab-overlay-private.h	\
			mnb-alttab-overlay-app.c	\
			mnb-alttab-overlay-app.h

libalttab_la_CPPFLAGS = $(alttab_cflags)
libalttab_la_LDFLAGS = $(LDADD)


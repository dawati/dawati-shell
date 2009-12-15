
noinst_LTLIBRARIES = libswitcher.la

AM_CFLAGS = \
	@MUTTER_PLUGIN_CFLAGS@ \
	-DMX_CACHE=\"$(MUTTER_MOBLIN_THEME_DIR)/mx.cache\" \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)\" \
	-I$(top_srcdir)/libmoblin-panel

libswitcher_la_LIBADD = $(MUTTER_PLUGIN_LIBS)
libswitcher_la_SOURCES =				\
			mnb-switcher-item.c             \
			mnb-switcher-item.h             \
			mnb-switcher-app.c              \
			mnb-switcher-app.h              \
			mnb-switcher-zone.c             \
			mnb-switcher-zone.h             \
			mnb-switcher-zone-apps.c        \
			mnb-switcher-zone-apps.h        \
			mnb-switcher-zone-new.c         \
			mnb-switcher-zone-new.h         \
			mnb-switcher-alttab.c		\
			mnb-switcher-alttab.h		\
			mnb-switcher-keys.c		\
			mnb-switcher-keys.h		\
			mnb-switcher.c                  \
			mnb-switcher.h			\
			mnb-switcher-private.h

libswitcher_la_CPPFLAGS = $(switcher_cflags)
libswitcher_la_LDFLAGS = $(LDADD)


noinst_LTLIBRARIES = libnotifications.la

AM_CFLAGS = \
	@MUTTER_PLUGIN_CFLAGS@ \
	-DMX_CACHE=\"$(MUTTER_MOBLIN_THEME_DIR)/mx.cache\" \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)\"

libnotifications_la_LIBADD = $(MUTTER_PLUGIN_LIBS)
libnotifications_la_SOURCES =				\
			$(DBUS_GLUE) 			\
			moblin-netbook-notify-store.c   \
			moblin-netbook-notify-store.h   \
                        mnb-notification.c              \
                        mnb-notification.h              \
                        mnb-notification-cluster.c      \
                        mnb-notification-cluster.h      \
                        mnb-notification-urgent.c       \
                        mnb-notification-urgent.h	\
			mnb-notification-gtk.c		\
			mnb-notification-gtk.h

DBUS_GLUE =	notification-manager-glue.h

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $^

libnotifications_la_CPPFLAGS = $(notifications_cflags)
libnotifications_la_LDFLAGS = $(LDADD)

BUILT_SOURCES = $(DBUS_GLUE)

CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = notification-manager.xml

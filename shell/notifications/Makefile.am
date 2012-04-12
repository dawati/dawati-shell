noinst_LTLIBRARIES = libnotifications.la

AM_CFLAGS = 						\
	$(DAWATI_SHELL_CFLAGS)	 			\
	-DMX_CACHE=\"$(DAWATI_THEME_DIR)/mx.cache\" 	\
	-DTHEMEDIR=\"$(DAWATI_RUNTIME_THEME_DIR)\"

libnotifications_la_LIBADD = $(DAWATI_SHELL_LIBS)
libnotifications_la_SOURCES =				\
			$(DBUS_GLUE) 			\
			dawati-netbook-notify-store.c   \
			dawati-netbook-notify-store.h   \
			ntf-notification.c		\
			ntf-notification.h		\
			ntf-overlay.c			\
			ntf-overlay.h			\
			ntf-source.c			\
			ntf-source.h			\
			ntf-tray.c			\
			ntf-tray.h			\
			ntf-libnotify.c			\
			ntf-libnotify.h			\
			ntf-wm.c			\
			ntf-wm.h			\
			ntf-gio.c			\
			ntf-gio.h

DBUS_GLUE =	notification-manager-glue.h

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $^

libnotifications_la_CPPFLAGS = $(notifications_cflags)
libnotifications_la_LDFLAGS = $(LDADD)

BUILT_SOURCES = $(DBUS_GLUE)

CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = notification-manager.xml

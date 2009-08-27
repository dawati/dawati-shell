SUBDIRS=tray effects switcher

pkglibdir=$(MUTTER_PLUGIN_DIR)

AM_CPPFLAGS = \
	@MUTTER_PLUGIN_CFLAGS@ \
	-DPLUGIN_LOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache\" \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)\"

moblin_netbook_la_SOURCES  = \
				mnb-scale-group.c	        \
				mnb-scale-group.h               \
				moblin-netbook.c		\
				moblin-netbook.h		\
				moblin-netbook-tray-manager.c	\
				moblin-netbook-tray-manager.h	\
				moblin-netbook-notify-store.c   \
				moblin-netbook-notify-store.h   \
				moblin-netbook-system-tray.h    \
				$(DBUS_BINDINGS) \
				$(DBUS_GLUE) \
				$(MARSHALS)                     \
				mnb-drop-down.h                 \
				mnb-drop-down.c                 \
				mnb-input-manager.h		\
				mnb-input-manager.c		\
				mnb-notification.c              \
				mnb-notification.h              \
				mnb-notification-cluster.c      \
				mnb-notification-cluster.h      \
				mnb-notification-urgent.c       \
				mnb-notification-urgent.h       \
				mnb-toolbar.c                   \
				mnb-toolbar.h                   \
				mnb-toolbar-button.c            \
				mnb-toolbar-button.h            \
				mnb-panel.c                     \
				mnb-panel.h

DBUS_GLUE =	notification-manager-glue.h	\
		mnb-panel-dbus-glue.h		\
		mnb-toolbar-dbus-glue.h

DBUS_BINDINGS =	mnb-panel-dbus-bindings.h	\
		mnb-toolbar-dbus-bindings.h

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $^

%-bindings.h: %.xml
	dbus-binding-tool --mode=glib-client --output=$@ --prefix=$(subst -,_,$*) $^

MARSHALS = marshal.c marshal.h
%.c: %.list
	echo '#include "marshal.h"' > $@ && \
	$(GLIB_GENMARSHAL) --prefix=moblin_netbook_marshal $^ --body >> $@
%.h: %.list
	$(GLIB_GENMARSHAL) --prefix=moblin_netbook_marshal $^ --header > $@

moblin_netbook_la_LDFLAGS  = $(PLUGIN_LT_LDFLAGS) -module -no-undefined
moblin_netbook_la_LIBADD   = @MUTTER_PLUGIN_LIBS@	\
				tray/libtray.la 	\
				effects/libeffects.la	\
				switcher/libswitcher.la

pkglib_LTLIBRARIES = moblin-netbook.la

# post-install hook to remove the .la and .a files we are not interested in
# (There is no way to stop libtool generating static libs locally, and we
# cannot do this globally because of libmetacity-private.so).
install-exec-hook:
	rm -f $(DESTDIR)$(pkglibdir)/*.a
	rm -f $(DESTDIR)$(pkglibdir)/*.la

BUILT_SOURCES = $(DBUS_GLUE) $(DBUS_BINDINGS) $(MARSHALS)

CLEANFILES = $(BUILT_SOURCES)

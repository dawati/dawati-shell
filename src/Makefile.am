SUBDIRS=tray effects

pkglibdir=$(MUTTER_PLUGIN_DIR)

INCLUDES = @MUTTER_PLUGIN_CFLAGS@ \
	-DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\" \
	-I$(top_srcdir)/penge

AM_CFLAGS   = -Werror=missing-prototypes -fPIC
moblin_netbook_la_SOURCES  = 	nutter/nutter-scale-group.c	\
				nutter/nutter-scale-group.h     \
				moblin-netbook.c		\
				moblin-netbook.h		\
				moblin-netbook-chooser.c	\
				moblin-netbook-chooser.h	\
				moblin-netbook-panel.c		\
				moblin-netbook-panel.h		\
				moblin-netbook-launcher.c       \
				moblin-netbook-launcher.h	\
				moblin-netbook-status.c		\
				moblin-netbook-status.h		\
				moblin-netbook-tray-manager.c	\
				moblin-netbook-tray-manager.h	\
				moblin-netbook-notify-store.c   \
				moblin-netbook-notify-store.h   \
                                moblin-netbook-system-tray.h    \
	                        $(DBUS_GLUE)                    \
				$(MARSHALS)                     \
				mnb-drop-down.h                 \
				mnb-drop-down.c                 \
				mnb-entry.c                     \
				mnb-entry.h                     \
				mnb-switcher.c                  \
				mnb-switcher.h                  \
				mnb-notification.c              \
				mnb-notification.h              \
				mnb-notification-cluster.c      \
				mnb-notification-cluster.h      \
				mnb-panel-button.c              \
				mnb-panel-button.h              \
				mnb-launcher-button.c           \
				mnb-launcher-button.h		\
				mnb-status-row.c		\
				mnb-status-row.h		\
				mnb-status-entry.c		\
				mnb-status-entry.h

DBUS_GLUE=notification-manager-glue.h
%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $^

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
				$(PENGE_LIBS)		\
				$(top_builddir)/penge/penge/libpenge.la

pkglib_LTLIBRARIES = moblin-netbook.la

# post-install hook to remove the .la and .a files we are not interested in
# (There is no way to stop libtool generating static libs locally, and we
# cannot do this globally because of libmetacity-private.so).
install-exec-hook:
	rm -f $(DESTDIR)$(pkglibdir)/*.a
	rm -f $(DESTDIR)$(pkglibdir)/*.la

BUILT_SOURCES = $(DBUS_GLUE) $(MARSHALS)
CLEANFILES = $(BUILT_SOURCES)

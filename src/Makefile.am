SUBDIRS=tray effects

pkglibdir=$(MUTTER_PLUGIN_DIR)

INCLUDES = @MUTTER_PLUGIN_CFLAGS@ @NETPANEL_CFLAGS@ @PEOPLE_CFLAGS@ \
	-DPREFIX=\"$(prefix)\" \
	-DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\" \
	-DMWB_PKGDATADIR=\"$(datadir)/moblin-web-browser\" \
	@AHOGHILL_CFLAGS@ \
	-I$(top_srcdir)/ahoghill \
	-I$(top_srcdir)/penge \
	-I$(top_srcdir)/src

AM_CFLAGS   = -fPIC
moblin_netbook_la_SOURCES  = 	mnb-scale-group.c	        \
				mnb-scale-group.h               \
				moblin-netbook.c		\
				moblin-netbook.h		\
				moblin-netbook-chooser.c	\
				moblin-netbook-chooser.h	\
				moblin-netbook-launcher.c       \
				moblin-netbook-launcher.h	\
				moblin-netbook-status.c		\
				moblin-netbook-status.h		\
				moblin-netbook-pasteboard.c	\
				moblin-netbook-pasteboard.h	\
				moblin-netbook-tray-manager.c	\
				moblin-netbook-tray-manager.h	\
				moblin-netbook-notify-store.c   \
				moblin-netbook-notify-store.h   \
                                moblin-netbook-system-tray.h    \
				mwb-utils.c			\
				mwb-utils.h			\
	                        $(DBUS_GLUE)                    \
				$(MARSHALS)                     \
				mnb-clipboard-item.c		\
				mnb-clipboard-item.h		\
				mnb-clipboard-store.c		\
				mnb-clipboard-store.h		\
				mnb-clipboard-view.c		\
				mnb-clipboard-view.h		\
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
				mnb-notification-urgent.c       \
				mnb-notification-urgent.h       \
                                mnb-toolbar.c                   \
                                mnb-toolbar.h                   \
				mnb-toolbar-button.c            \
				mnb-toolbar-button.h            \
				mnb-launcher-button.c           \
				mnb-launcher-button.h		\
				mnb-launcher-tree.c \
				mnb-launcher-tree.h \
				mnb-status-row.c		\
				mnb-status-row.h		\
				mnb-status-entry.c		\
				mnb-status-entry.h
#				mnb-panel.c			\
#				mnb-panel.h

if WITH_NETPANEL
moblin_netbook_la_SOURCES += 	moblin-netbook-netpanel.c	\
				moblin-netbook-netpanel.h
endif

if WITH_PEOPLE
moblin_netbook_la_SOURCES +=	moblin-netbook-people.h		\
				moblin-netbook-people.c
endif

#DBUS_GLUE=notification-manager-glue.h mnb-panel-dbus-glue.h
DBUS_GLUE=notification-manager-glue.h
%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $^

#DBUS_BINDINGS=mnb-panel-dbus-bindings.h
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
				@NETPANEL_LIBS@		\
				@PEOPLE_LIBS@		\
				tray/libtray.la 	\
				effects/libeffects.la	\
				$(top_builddir)/penge/penge/libpenge.la \
				$(PENGE_LIBS)

if WITH_AHOGHILL
moblin_netbook_la_LIBADD += 				\
				$(AHOGHILL_LIBS)	\
				$(top_builddir)/ahoghill/ahoghill/libahoghill.la
endif



pkglib_LTLIBRARIES = moblin-netbook.la

# post-install hook to remove the .la and .a files we are not interested in
# (There is no way to stop libtool generating static libs locally, and we
# cannot do this globally because of libmetacity-private.so).
install-exec-hook:
	rm -f $(DESTDIR)$(pkglibdir)/*.a
	rm -f $(DESTDIR)$(pkglibdir)/*.la

BUILT_SOURCES = $(MARSHALS) $(DBUS_GLUE) $(DBUS_BINDINGS)
CLEANFILES = $(BUILT_SOURCES)

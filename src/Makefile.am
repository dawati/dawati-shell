SUBDIRS=tray effects

pkglibdir=$(MUTTER_PLUGIN_DIR)

INCLUDES = @MUTTER_PLUGIN_CFLAGS@ @NETPANEL_CFLAGS@ \
	-DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\" \
	-DMWB_PKGDATADIR=\"$(datadir)/moblin-web-browser\" \
	-I$(top_srcdir)/ahoghill \
	-I$(top_srcdir)/penge

AM_CFLAGS   = -fPIC
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
				mwb-radical-bar.c		\
				mwb-radical-bar.h		\
				mwb-utils.c			\
				mwb-utils.h			\
	                        $(DBUS_GLUE)                    \
				$(MARSHALS)                     \
				mnb-drop-down.h                 \
				mnb-drop-down.c                 \
				mnb-entry.c                     \
				mnb-entry.h                     \
				mnb-expander.c                  \
				mnb-expander.h                  \
				mnb-switcher.c                  \
				mnb-switcher.h                  \
				mnb-notification.c              \
				mnb-notification.h              \
				mnb-notification-cluster.c      \
				mnb-notification-cluster.h      \
				mnb-notification-urgent.c       \
				mnb-notification-urgent.h       \
				mnb-panel-button.c              \
				mnb-panel-button.h              \
				mnb-launcher-button.c           \
				mnb-launcher-button.h		\
				mnb-launcher-tree.c \
				mnb-launcher-tree.h \
				mnb-status-row.c		\
				mnb-status-row.h		\
				mnb-status-entry.c		\
				mnb-status-entry.h

if WITH_NETPANEL
moblin_netbook_la_SOURCES += 	moblin-netbook-netpanel.c	\
				moblin-netbook-netpanel.h
endif



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
				@NETPANEL_LIBS@		\
				tray/libtray.la 	\
				effects/libeffects.la	\
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

BUILT_SOURCES = $(DBUS_GLUE) $(MARSHALS)
CLEANFILES = $(BUILT_SOURCES)


pkglibdir=$(MUTTER_PLUGIN_DIR)

INCLUDES=@MUTTER_PLUGIN_CFLAGS@ -DPLUGIN_PKGDATADIR=\"$(pkgdatadir)\"

AM_CFLAGS   = -Werror=missing-prototypes -fPIC
moblin_netbook_la_SOURCES  = 	nutter/nutter-scale-group.c	\
				nutter/nutter-scale-group.h     \
				moblin-netbook.c		\
				moblin-netbook.h		\
				moblin-netbook-ui.c		\
				moblin-netbook-ui.h		\
				moblin-netbook-chooser.c	\
				moblin-netbook-chooser.h	\
				moblin-netbook-switcher.c	\
				moblin-netbook-switcher.h	\
				moblin-netbook-panel.c		\
				moblin-netbook-panel.h		\
				moblin-netbook-launcher.c       \
				moblin-netbook-launcher.h       \
				moblin-netbook-notify-store.c   \
				moblin-netbook-notify-store.h   \
                                tidy-behaviour-bounce.c		\
                                tidy-behaviour-bounce.h         \
				moblin-netbook-notify.c         \
	                        $(DBUS_GLUE)                    \
				$(MARSHALS)

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
moblin_netbook_la_LIBADD   = @MUTTER_PLUGIN_LIBS@

pkglib_LTLIBRARIES = moblin-netbook.la

# post-install hook to remove the .la and .a files we are not interested in
# (There is no way to stop libtool generating static libs locally, and we
# cannot do this globally because of libmetacity-private.so).
install-exec-hook:
	rm -f $(DESTDIR)$(pkglibdir)/*.a
	rm -f $(DESTDIR)$(pkglibdir)/*.la

BUILT_SOURCES = $(DBUS_GLUE) $(MARSHALS)
CLEANFILES = $(BUILT_SOURCES)

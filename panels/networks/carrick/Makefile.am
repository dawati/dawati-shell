# connman-marshal defines the marshalling code used for the signals sourced
# by ConnMan
connman-marshal.c: connman-marshal.list Makefile
	$(AM_V_GEN)glib-genmarshal --prefix=connman_marshal --body $< > $@

connman-marshal.h: connman-marshal.list Makefile
	$(AM_V_GEN)glib-genmarshal --prefix=connman_marshal --header $< > $@

%-bindings.h: %.xml
	$(AM_V_GEN)dbus-binding-tool --mode=glib-client --output=$@ --prefix=$(subst -,_,$*) $^

BUILT_SOURCES = connman-marshal.c connman-marshal.h \
                connman-manager-bindings.h \
                connman-service-bindings.h \
                connman-technology-bindings.h

noinst_LTLIBRARIES = \
                        libcarrick.la

libcarrick_la_SOURCES = carrick-applet.c \
                        carrick-applet.h \
                        carrick-pane.c \
                        carrick-pane.h \
                        carrick-icon-factory.c \
                        carrick-icon-factory.h \
                        carrick-service-item.c \
                        carrick-service-item.h \
                        carrick-list.c \
                        carrick-list.h \
                        carrick-notification-manager.c \
                        carrick-notification-manager.h \
                        carrick-network-model.c \
                        carrick-network-model.h \
                        carrick-util.c \
                        carrick-util.h \
			carrick-shell.h \
                        mux-banner.c \
                        mux-banner.h \
			nbtk-gtk-expander.c \
			nbtk-gtk-expander.h \
                        $(BUILT_SOURCES) \
                        $(NULL)

libcarrick_la_CFLAGS =  -Wall \
                        $(GTK_CFLAGS) \
                        $(GCONF_CFLAGS) \
                        $(MPL_CFLAGS) \
                        $(MX_CFLAGS) \
                        $(LIBNOTIFY_CFLAGS) \
                        -DPKG_DATA_DIR=\"$(pkgdatadir)\" \
                        -DLIBEXECDIR=\"$(libexecdir)\"

libcarrick_la_LIBADD = $(GTK_LIBS) \
                       $(GCONF_LIBS) \
                       $(MPL_LIBS) \
                       $(MX_LIBS) \
                       $(LIBNOTIFY_LIBS)



DISTCLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = connman-marshal.list \
             connman-manager.xml \
             connman-service.xml \
             connman-technology.xml

-include $(top_srcdir)/git.mk

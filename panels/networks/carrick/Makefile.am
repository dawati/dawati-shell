# connman-marshal defines the marshalling code used for the signals sourced
# by ConnMan
connman-marshal.c: connman-marshal.list Makefile
	$(AM_V_GEN)glib-genmarshal --prefix=connman_marshal --body $< > $@

connman-marshal.h: connman-marshal.list Makefile
	$(AM_V_GEN)glib-genmarshal --prefix=connman_marshal --header $< > $@

%-bindings.h: %.xml
	$(AM_V_GEN)dbus-binding-tool --mode=glib-client --output=$@ --prefix=$(subst -,_,$*) $^

BUILT_SOURCES = connman-marshal.c connman-marshal.h \
                connman-manager-bindings.h connman-service-bindings.h

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
												$(BUILT_SOURCES) \
                        $(NULL)

if ! HAVE_INFOBAR
libcarrick_la_SOURCES += gtkinfobar.h gtkinfobar.c
endif

libcarrick_la_CFLAGS =  -Wall \
                        $(GTK_CFLAGS) \
                        $(NBTK_CFLAGS) \
                        $(LIBNOTIFY_CFLAGS) \
                        -DPKG_DATA_DIR=\"$(pkgdatadir)\" \
                        -DLIBEXECDIR=\"$(libexecdir)\"

libcarrick_la_LIBADD = $(GTK_LIBS) \
                       $(NBTK_LIBS) \
                       $(LIBNOTIFY_LIBS)



DISTCLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = connman-marshal.list connman-manager.xml connman-service.xml

-include $(top_srcdir)/git.mk

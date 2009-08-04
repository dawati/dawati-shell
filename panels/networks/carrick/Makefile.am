# connman-marshal defines the marshalling code used for the signals sourced
# by ConnMan
MARSHALFILES = connman-marshal.c connman-marshal.h
BUILT_SOURCES = $(MARSHALFILES)

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
                        carrick-status-icon.c \
                        carrick-status-icon.h \
                        carrick-list.c \
                        carrick-list.h \
			carrick-notification-manager.c \
			carrick-notification-manager.h \
			carrick-network-model.c \
			carrick-network-model.h \
                        $(NULL)

libcarrick_la_CFLAGS =  -Wall \
                        $(GCONNMAN_CFLAGS) \
                        $(GTK_CFLAGS) \
                        $(NBTK_CFLAGS) \
                        $(LIBNOTIFY_CFLAGS) \
                        -DPKG_DATA_DIR=\"$(pkgdatadir)\"

libcarrick_la_LIBADD = $(GCONNMAN_LIBS) \
                       $(GTK_LIBS) \
                       $(NBTK_LIBS) \
                       $(LIBNOTIFY_LIBS)

connman-marshal.c: connman-marshal.list Makefile
	glib-genmarshal --prefix=connman_marshal --body $< > $@

connman-marshal.h: connman-marshal.list Makefile
	glib-genmarshal --prefix=connman_marshal --header $< > $@

DISTCLEANFILES = $(MARSHALFILES)
EXTRA_DIST = connman-marshal.list

-include $(top_srcdir)/git.mk

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
                        $(NULL)

libcarrick_la_CFLAGS =  -Wall -I$(top_srcdir) \
                        $(GCONNMAN_CFLAGS) \
                        $(GTK_CFLAGS) \
                        $(LIBNOTIFY_CFLAGS) \
                        -DPKG_DATA_DIR=\"$(pkgdatadir)\"

libcarrick_la_LIBADD = $(top_builddir)/mux/libmux.la \
                       $(GCONNMAN_LIBS) \
                       $(GTK_LIBS) \
                       $(LIBNOTIFY_LIBS)

-include $(top_srcdir)/git.mk

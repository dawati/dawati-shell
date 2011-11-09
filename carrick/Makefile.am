# connman-marshal defines the marshalling code used for the signals sourced
# by ConnMan
connman-marshal.c: connman-marshal.list Makefile
	$(AM_V_GEN)glib-genmarshal --prefix=connman_marshal --body $< > $@

connman-marshal.h: connman-marshal.list Makefile
	$(AM_V_GEN)glib-genmarshal --prefix=connman_marshal --header $< > $@

%-bindings.h: %.xml
	$(AM_V_GEN)dbus-binding-tool --mode=glib-client --output=$@ --prefix=$(subst -,_,$*) $^

BUILT_SOURCES = \
	connman-marshal.c \
	connman-marshal.h \
	connman-manager-bindings.h \
	connman-service-bindings.h \
	connman-technology-bindings.h \
	$(NULL)

noinst_LTLIBRARIES = \
	libcarrick.la \
	$(NULL)

libcarrick_la_SOURCES = \
	carrick-applet.c \
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

libcarrick_la_CFLAGS = \
        $(CARRICK_CFLAGS) \
	-Wall \
	-DPKG_DATA_DIR=\"$(PANEL_NETWORKS_DATADIR)\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	$(NULL)

libcarrick_la_LIBADD = \
	$(CARRICK_LIBS) \
	$(NULL)

DISTCLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	connman-marshal.list \
	connman-manager.xml \
	connman-service.xml \
	connman-technology.xml \
	$(NULL)

-include $(top_srcdir)/git.mk

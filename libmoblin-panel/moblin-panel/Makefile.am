
lib_LTLIBRARIES = libmoblin-panel.la

libmoblin_panel_includedir = $(includedir)/libmoblin-panel/moblin-panel
libmoblin_panel_include_HEADERS = \
	mpl-entry.h \
	mpl-icon-theme.h \
	mpl-panel-client.h \
	mpl-panel-clutter.h \
	mpl-panel-common.h \
	mpl-panel-gtk.h \
	mpl-app-bookmark-manager.h \
	mpl-utils.h

libmoblin_panel_la_CFLAGS = \
	$(LIBMPL_CFLAGS) \
	-DNBTK_CACHE=\"$(MUTTER_MOBLIN_THEME_DIR)/moblin-panel/nbtk.cache\" \
	-DDATADIR=\"$(datadir)\" \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/moblin-panel\"

libmoblin_panel_la_LDFLAGS = $(LIBMPL_LIBS)

libmoblin_panel_la_SOURCES = \
	mpl-entry.c \
	mpl-icon-theme.c \
	mpl-panel-client.c \
	mpl-panel-clutter.c \
	mpl-panel-gtk.c \
	mpl-app-bookmark-manager.c \
	mpl-utils.c \
	$(DBUS_GLUE) \
	$(DBUS_BINDINGS) \
	$(MARSHALS)

DBUS_GLUE = mnb-panel-dbus-glue.h		\
	    mnb-toolbar-dbus-glue.h

DBUS_BINDINGS =	mnb-panel-dbus-bindings.h	\
		mnb-toolbar-dbus-bindings.h

%-glue.h: $(top_srcdir)/src/%.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $^

%-bindings.h: $(top_srcdir)/src/%.xml
	dbus-binding-tool --mode=glib-client --output=$@ --prefix=$(subst -,_,$*) $^

MARSHALS = marshal.c marshal.h
%.c: $(top_srcdir)/src/%.list
	echo '#include "marshal.h"' > $@ && \
	$(GLIB_GENMARSHAL) --prefix=moblin_netbook_marshal $^ --body >> $@
%.h: $(top_srcdir)/src/%.list
	$(GLIB_GENMARSHAL) --prefix=moblin_netbook_marshal $^ --header > $@

BUILT_SOURCES = $(DBUS_GLUE) $(DBUS_BINDINGS) $(MARSHALS)

CLEANFILES = $(BUILT_SOURCES)


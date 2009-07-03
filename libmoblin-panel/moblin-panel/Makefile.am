
lib_LTLIBRARIES = libmoblin-panel-gtk.la libmoblin-panel-clutter.la

libmoblin_panel_includedir = $(includedir)/libmoblin-panel/moblin-panel
libmoblin_panel_include_HEADERS = \
	mpl-entry.h \
	mpl-panel-client.h \
	mpl-panel-clutter.h \
	mpl-panel-common.h \
	mpl-panel-gtk.h

libmoblin_panel_clutter_la_CFLAGS = \
	$(LIBMPL_CLUTTER_CFLAGS) \
	-DNBTK_CACHE=\"$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache\" \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/moblin-panel\"

# FIXME use this once split out
# -DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)/$(PACKAGE_NAME)\"

libmoblin_panel_clutter_la_LDFLAGS = $(LIBMPL_CLUTTER_LIBS)

libmoblin_panel_clutter_la_SOURCES = \
	mpl-clutter-theme.c \
	mpl-clutter-theme.h \
	mpl-entry.c \
	mpl-panel-client.c \
	mpl-panel-clutter.c \
	$(DBUS_GLUE) \
	$(DBUS_BINDINGS) \
	$(MARSHALS)

libmoblin_panel_gtk_la_CFLAGS = $(LIBMPL_GTK_CFLAGS)

libmoblin_panel_gtk_la_LDFLAGS = $(LIBMPL_GTK_LIBS)

libmoblin_panel_gtk_la_SOURCES = \
	mpl-panel-client.c \
	mpl-panel-gtk.c \
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


SUBDIRS=effects switcher notifications

xmldir = $(pkgdatadir)/dbus-xml
xml_DATA = 	mnb-panel-dbus.xml	\
		mnb-toolbar-dbus.xml

pkglibdir=$(MUTTER_PLUGIN_DIR)

AM_CFLAGS = \
	$(MUTTER_PLUGIN_CFLAGS) \
	-DPLUGIN_LOCALEDIR=\"$(localedir)\" \
	-DNBTK_CACHE=\"$(MUTTER_MOBLIN_THEME_DIR)/nbtk.cache\" \
	-DTHEMEDIR=\"$(MUTTER_MOBLIN_THEME_DIR)\" \
	-I$(top_srcdir)/libmoblin-panel


moblin_netbook_la_SOURCES  = \
				mnb-scale-group.c	        \
				mnb-scale-group.h               \
				moblin-netbook.c		\
				moblin-netbook.h		\
				$(DBUS_BINDINGS) \
				$(DBUS_GLUE) \
				$(MARSHALS)                     \
				mnb-drop-down.h                 \
				mnb-drop-down.c                 \
				mnb-input-manager.h		\
				mnb-input-manager.c		\
				mnb-toolbar.c                   \
				mnb-toolbar.h                   \
				mnb-toolbar-button.c            \
				mnb-toolbar-button.h            \
				mnb-panel.c         		\
				mnb-panel.h			\
				mnb-panel-frame.c         	\
				mnb-panel-frame.h		\
				mnb-panel-oop.c                 \
				mnb-panel-oop.h

DBUS_GLUE =	mnb-panel-dbus-glue.h		\
		mnb-toolbar-dbus-glue.h

DBUS_BINDINGS =	mnb-panel-dbus-bindings.h	\
		mnb-toolbar-dbus-bindings.h

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $^

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
				effects/libeffects.la	\
				switcher/libswitcher.la \
				notifications/libnotifications.la

pkglib_LTLIBRARIES = moblin-netbook.la

# post-install hook to remove the .la and .a files we are not interested in
# (There is no way to stop libtool generating static libs locally, and we
# cannot do this globally because of libmetacity-private.so).
install-exec-hook:
	rm -f $(DESTDIR)$(pkglibdir)/*.a
	rm -f $(DESTDIR)$(pkglibdir)/*.la

BUILT_SOURCES = $(DBUS_GLUE) $(DBUS_BINDINGS) $(MARSHALS)

CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = marshal.list mnb-panel-dbus.xml mnb-toolbar-dbus.xml

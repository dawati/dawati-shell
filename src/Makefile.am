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

source_h =	../libmoblin-panel/moblin-panel/mpl-panel-common.h \
		$(DBUS_GLUE) 			\
		$(DBUS_BINDINGS) 		\
		mnb-scale-group.h               \
		moblin-netbook.h		\
		mnb-drop-down.h                 \
		mnb-input-manager.h		\
		mnb-toolbar.h                   \
		mnb-toolbar-button.h            \
		mnb-panel.h			\
		mnb-panel-frame.h		\
		mnb-panel-oop.h


source_c = 	mnb-scale-group.c	        \
		moblin-netbook.c		\
		$(MARSHALS)                     \
		mnb-drop-down.c                 \
		mnb-input-manager.c		\
		mnb-toolbar.c                   \
		mnb-toolbar-button.c            \
		mnb-panel.c         		\
		mnb-panel-frame.c         	\
		mnb-panel-oop.c			\
		mnb-enum-types.c

moblin_netbook_la_SOURCES  = 	$(source_h) \
				$(source_c)

DBUS_GLUE =	mnb-panel-dbus-glue.h		\
		mnb-toolbar-dbus-glue.h

DBUS_BINDINGS =	mnb-panel-dbus-bindings.h	\
		mnb-toolbar-dbus-bindings.h

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $^

%-bindings.h: %.xml
	dbus-binding-tool --mode=glib-client --output=$@ --prefix=$(subst -,_,$*) $^

STAMPS = stamp-marshal.h stamp-mnb-enum-types.h

ENUMS = mnb-enum-types.h \
	mnb-enum-types.c

mnb-enum-types.h: stamp-mnb-enum-types.h
	@true
stamp-mnb-enum-types.h: $(source_h) Makefile
	$(QUIET_GEN)( $(GLIB_MKENUMS) \
	  	--template $(srcdir)/mnb-enum-types.h.in \
	  $(source_h) ) >> xgen-ceth && \
	(cmp -s xgen-ceth mnb-enum-types.h || cp xgen-ceth mnb-enum-types.h) && \
	rm -f xgen-ceth && \
	echo timestamp > $(@F)

mnb-enum-types.c: mnb-enum-types.h
	$(QUIET_GEN)( $(GLIB_MKENUMS) \
	  	--template $(srcdir)/mnb-enum-types.c.in \
	  $(source_h) ) >> xgen-cetc && \
	cp xgen-cetc mnb-enum-types.c && \
	rm -f xgen-cetc

MARSHALS = marshal.c marshal.h

marshal.h: stamp-marshal.h
	@true
stamp-marshal.h: marshal.list
	$(QUIET_GEN)$(GLIB_GENMARSHAL) \
		--prefix=moblin_netbook_marshal \
		--header \
	  $(srcdir)/marshal.list > xgen-cmh \
	&& (cmp -s xgen-cmh marshal.h || cp xgen-cmh marshal.h ) \
	&& rm -f xgen-cmh \
	&& echo timestamp > $(@F)

mnb-marshal.c: marshal.h Makefile
	$(QUIET_GEN)( echo "#include \"marshal.h\"" ; \
	  $(GLIB_GENMARSHAL) \
	  	--prefix=moblin_netbook_marshal \
		--body \
	  $(srcdir)/marshal.list --body ) >> xgen-cmc \
	&& cp xgen-cmc marshal.c \
	&& rm -f xgen-cmc

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

BUILT_SOURCES = $(DBUS_GLUE) $(DBUS_BINDINGS) $(MARSHALS) $(ENUMS)

CLEANFILES = $(BUILT_SOURCES) $(STAMPS)
EXTRA_DIST =	marshal.list \
		mnb-panel-dbus.xml \
		mnb-toolbar-dbus.xml \
		mnb-enum-types.h.in \
		mnb-enum-types.c.in


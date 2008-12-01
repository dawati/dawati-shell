tray_cflags =					\
	$(MUTTER_PLUGIN_CFLAGS)			\
	-DG_DISABLE_DEPRECATED			\
	-DG_LOG_DOMAIN=\"notification_area\"	\
	$(TRAY_CFLAGS)				\
	$(NULL)

tray_built_sources = \
	na-marshal.h \
	na-marshal.c

TRAY_STAMP_FILES = stamp-na-marshal.h

# please, keep this sorted alphabetically
tray_source =			\
	na-tray-child.c		\
	na-tray-child.h		\
	na-tray-manager.c	\
	na-tray-manager.h	\
	$(NULL)

na-marshal.h: stamp-na-marshal.h
	@true

stamp-na-marshal.h: Makefile na-marshal.list
	$(GLIB_GENMARSHAL) \
		--prefix=_na_marshal \
		--header \
	$(top_srcdir)/src/tray/na-marshal.list > xgen-tmh && \
	(cmp -s xgen-tmh na-marshal.h || cp -f xgen-tmh na-marshal.h) && \
	rm -f xgen-tmh && \
	echo timestamp > $(@F)

na-marshal.c: Makefile na-marshal.list na-marshal.h
	(echo "#include \"na-marshal.h\"" ; \
	 $(GLIB_GENMARSHAL) \
		--prefix=_na_marshal \
		--body \
	 $(top_srcdir)/src/tray/na-marshal.list ) > xgen-tmc && \
	cp -f xgen-tmc na-marshal.c && \
	rm -f xgen-tmc

noinst_LTLIBRARIES = libtray.la

libtray_la_LIBADD = $(MUTTER_PLUGIN_LIBS)
libtray_la_SOURCES =		\
	$(tray_built_sources)	\
	$(tray_source)		\
	$(NULL)
libtray_la_CPPFLAGS = $(tray_cflags)
libtray_la_LDFLAGS = $(LDADD)

CLEANFILES = $(TRAY_STAMP_FILES)

EXTRA_DIST = na-marshal.list

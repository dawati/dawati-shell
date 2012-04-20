privlibdir = $(pkglibdir)/mutter-stub
privlib_LTLIBRARIES = libmutter-stub.la

mutter_built_sources = \
	mutter-enum-types.h \
	mutter-enum-types.c

libmutter_stub_la_CPPFLAGS =		\
	-I.				\
	$(DAWATI_SHELL_CFLAGS)		\
	$(MUTTER_STUB_CFLAGS)		\
	-DG_LOG_DOMAIN="\"Mutter-Stub\""

libmutter_stub_la_LIBADD =		\
	$(DAWATI_SHELL_LIBS)		\
	$(MUTTER_STUB_LIBS)

libmutter_stub_la_SOURCES =		\
	mutter-classes.c		\
	mutter-stub.c			\
	meta/boxes.h			\
	meta/common.h			\
	meta/compositor.h		\
	meta/compositor-mutter.h	\
	meta/display.h			\
	meta/meta-background-actor.h	\
	meta/meta-plugin.h		\
	meta/meta-window-actor.h	\
	meta/prefs.h			\
	meta/screen.h			\
	meta/types.h			\
	meta/window.h			\
	meta/workspace.h		\
	$(mutter_built_sources)

EXTRA_DIST = \
	mutter-enum-types.h.in \
	mutter-enum-types.c.in \
	meta/atomnames.h \
	meta/boxes.h \
	meta/common.h \
	meta/compositor.h \
	meta/compositor-mutter.h \
	meta/display.h \
	meta/errors.h \
	meta/gradient.h \
	meta/group.h \
	meta/keybindings.h \
	meta/main.h \
	meta/meta-background-actor.h \
	meta/meta-plugin.h \
	meta/meta-shadow-factory.h \
	meta/meta-shaped-texture.h \
	meta/meta-window-actor.h \
	meta/prefs.h \
	meta/preview-widget.h \
	meta/screen.h \
	meta/theme.h \
	meta/types.h \
	meta/util.h \
	meta/window.h \
	meta/workspace.h


BUILT_SOURCES = $(mutter_built_sources)

MUTTER_STAMP_FILES = stamp-mutter-enum-types.h
CLEANFILES = $(MUTTER_STAMP_FILES) $(mutter_built_sources)

mutter-enum-types.h: stamp-mutter-enum-types.h Makefile
	@true
stamp-mutter-enum-types.h: $(libmutterinclude_base_headers) mutter-enum-types.h.in
	$(AM_V_GEN) ( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	    --template mutter-enum-types.h.in \
	  meta/*.h ) >> xgen-teth && \
	(cmp -s xgen-teth mutter-enum-types.h || cp xgen-teth mutter-enum-types.h) && \
	rm -f xgen-teth && \
	echo timestamp > $(@F)

mutter-enum-types.c: stamp-mutter-enum-types.h mutter-enum-types.c.in
	  $(AM_V_GEN) ( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	    --template mutter-enum-types.c.in \
	  meta/*.h ) >> xgen-tetc && \
	cp xgen-tetc mutter-enum-types.c && \
	rm -f xgen-tetc

-include $(INTROSPECTION_MAKEFILE)

INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS =

typelibdir = $(pkglibdir)/mutter-stub
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

girdir = $(pkgdatadir)/gir-1.0/mutter-stub
gir_DATA = $(INTROSPECTION_GIRS)

Meta-3.0.gir: libmutter-stub.la
Meta_3_0_gir_INCLUDES = Clutter-1.0 ClutterX11-1.0 Gtk-3.0
Meta_3_0_gir_CFLAGS = $(libmutter_stub_la_CPPFLAGS)
Meta_3_0_gir_LIBS = libmutter-stub.la
Meta_3_0_gir_FILES = $(libmutter_stub_la_SOURCES)
INTROSPECTION_GIRS += Meta-3.0.gir
CLEANFILES += Meta-3.0.gir

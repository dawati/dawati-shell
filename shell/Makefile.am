SUBDIRS = effects alttab notifications presence js

bin_PROGRAMS = dawati-shell

privlibdir = $(pkglibdir)
privlib_LTLIBRARIES = libdawati-shell.la libdawati-shell-js.la

########################################

shell_built_sources =		\
	marshal.h		\
	$(DBUS_GLUE) 		\
	$(DBUS_BINDINGS)	\
	$(NULL)

shell_private_sources = \
	gactionmuxer.h				\
	gactionmuxer.c				\
	gactionobservable.h			\
	gactionobservable.c			\
	gactionobserver.h			\
	gactionobserver.c			\
	$(top_srcdir)/libdawati-panel/dawati-panel/mpl-panel-common.h \
	dawati-netbook.h			\
	dawati-netbook-mutter-hints.h		\
	mnb-spinner.h				\
	mnb-input-manager.h			\
	mnb-statusbar.h         	      	\
	mnb-toolbar.h                 		\
	mnb-toolbar-applet.h          		\
	mnb-toolbar-background.h      		\
	mnb-toolbar-button.h          		\
	mnb-toolbar-clock.h           		\
	mnb-toolbar-icon.h            		\
	mnb-toolbar-shadow.h          		\
	mnb-panel.h				\
	mnb-panel-frame.h			\
	mnb-panel-oop.h				\
	dawati-netbook.c			\
	dawati-netbook-mutter-hints.c		\
	mnb-spinner.c				\
	marshal.c                   		\
	mnb-input-manager.c			\
	mnb-statusbar.c               		\
	mnb-toolbar.c                 		\
	mnb-toolbar-applet.c          		\
	mnb-toolbar-background.c      		\
	mnb-toolbar-button.c          		\
	mnb-toolbar-icon.c           	 	\
	mnb-toolbar-clock.c           		\
	mnb-toolbar-shadow.c          		\
	mnb-panel.c         			\
	mnb-panel-frame.c         		\
	mnb-panel-oop.c				\
	dawati-shell-plugin.c

shell_public_headers_h =			\
						\
	shell-a11y.h				\
	shell-app.h				\
	shell-app-system.h			\
	shell-app-usage.h			\
	shell-generic-container.h		\
	shell-global.h				\
	shell-perf-log.h			\
	shell-util.h				\
	shell-window-tracker.h			\
	shell-wm.h				\
	$(NULL)


libdawati_shell_la_SOURCES =			\
	$(shell_private_sources)		\
	$(shell_built_sources)			\
	$(shell_public_headers_h)		\
	\
	mnb-enum-types.c			\
	mnb-enum-types.h			\
	\
	shell-a11y.c				\
	shell-app-private.h			\
	shell-app-system-private.h		\
	shell-app.c				\
	shell-app-system.c			\
	shell-app-usage.c			\
	shell-generic-container.c		\
	shell-global-private.h			\
	shell-global.c				\
	shell-jsapi-compat-private.h		\
	shell-perf-log.c			\
	shell-util.c				\
	shell-window-tracker-private.h		\
	shell-window-tracker.c			\
	shell-wm-private.h			\
	shell-wm.c				\
	$(NULL)

libdawati_shell_la_CPPFLAGS =				\
	$(DAWATI_SHELL_CFLAGS) 				\
	-DPLUGIN_LOCALEDIR=\"$(localedir)\" 		\
	-DMX_CACHE=\"$(DAWATI_THEME_DIR)/mx.cache\" 	\
	-DTHEMEDIR=\"$(DAWATI_RUNTIME_THEME_DIR)\" 	\
	-DPANELSDIR=\"$(DAWATI_PANELS_DIR)\" 		\
	-DJSDIR=\"$(pkgdatadir)/js\"			\
	-DDAWATI_SHELL_DATADIR=\"$(pkgdatadir)\"	\
	-I$(top_srcdir)/libdawati-panel			\
	-I$(top_srcdir)/st

libdawati_shell_la_LDFLAGS  = 				\
	-avoid-version

libdawati_shell_la_LIBADD = 						\
	$(DAWATI_SHELL_LIBS)						\
	$(BLUETOOTH_LIBS)						\
	$(top_builddir)/gvc/libgvc.la					\
	$(top_builddir)/st/libst-1.0.la					\
	effects/libeffects.la						\
	alttab/libalttab.la						\
	presence/libpresence.la						\
	notifications/libnotifications.la				\
	$(top_builddir)/libdawati-panel/dawati-panel/libdawati-panel.la

libdawati_shell_la_gir_sources = \
	$(filter-out %-private.h $(shell_private_sources) $(shell_built_sources), $(shell_public_headers_h) $(libdawati_shell_la_SOURCES))

########################################

libdawati_shell_js_la_SOURCES =		\
	shell-js.h			\
	shell-js.c			\
	$(NULL)

libdawati_shell_js_la_LIBADD =		\
	$(DAWATI_SHELL_JS_LIBS)		\
	$(NULL)

libdawati_shell_js_la_LDFLAGS =		\
	-avoid-version

libdawati_shell_js_la_CPPFLAGS =	\
	$(DAWATI_SHELL_JS_CFLAGS)

########################################

dawati_shell_SOURCES = 	\
	main.c		\
	$(NULL)

dawati_shell_CPPFLAGS =					\
	$(DAWATI_SHELL_CFLAGS)				\
	-DLOCALEDIR=\"$(localedir)\"	 		\
	-DMX_CACHE=\"$(DAWATI_THEME_DIR)/mx.cache\" 	\
	-DTHEMEDIR=\"$(DAWATI_RUNTIME_THEME_DIR)\" 	\
	-DPANELSDIR=\"$(DAWATI_PANELS_DIR)\"		\
	-DDAWATI_SHELL_PKGLIBDIR=\"$(pkglibdir)\"

dawati_shell_LDADD =	 				\
	libdawati-shell.la				\
	$(DAWATI_SHELL_LIBS)				\
	$(libdawati_shell_la_LIBADD)

dawati_shell_DEPENDENCIES =				\
	libdawati-shell.la

########################################

DBUS_GLUE = \
	mnb-panel-dbus-glue.h		\
	mnb-toolbar-dbus-glue.h		\
	$(NULL)

DBUS_BINDINGS =	\
	mnb-panel-dbus-bindings.h	\
	mnb-toolbar-dbus-bindings.h	\
	$(NULL)

xmldir = $(pkgdatadir)/dbus-xml
xml_DATA =			\
	mnb-panel-dbus.xml	\
	mnb-toolbar-dbus.xml	\
	$(NULL)

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $^

%-bindings.h: %.xml
	dbus-binding-tool --mode=glib-client --output=$@ --prefix=$(subst -,_,$*) $^

STAMPS = stamp-marshal.h stamp-mnb-enum-types.h

ENUMS = \
	mnb-enum-types.h	\
	mnb-enum-types.c	\
	$(NULL)

mnb-enum-types.h: stamp-mnb-enum-types.h
	@true
stamp-mnb-enum-types.h: $(shell_public_headers_h) Makefile
	$(AM_V_GEN) ( $(GLIB_MKENUMS) \
		--template mnb-enum-types.h.in \
	  $(shell_public_headers_h) $(filter %.h, $(shell_private_sources)) ) >> xgen-ceth && \
	(cmp -s xgen-ceth mnb-enum-types.h || cp xgen-ceth mnb-enum-types.h) && \
	rm -f xgen-ceth && \
	echo timestamp > $(@F)

mnb-enum-types.c: mnb-enum-types.h
	$(AM_V_GEN) ( $(GLIB_MKENUMS) \
		--template mnb-enum-types.c.in \
	  $(shell_public_headers_h) $(filter %.h, $(shell_private_sources)) ) >> xgen-cetc && \
	cp xgen-cetc mnb-enum-types.c && \
	rm -f xgen-cetc

MARSHALS = marshal.c marshal.h

marshal.h: stamp-marshal.h
	@true
stamp-marshal.h: marshal.list
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) \
		--prefix=dawati_netbook_marshal \
		--header \
	  marshal.list ) > xgen-cmh \
	&& (cmp -s xgen-cmh marshal.h || cp xgen-cmh marshal.h ) \
	&& rm -f xgen-cmh \
	&& echo timestamp > $(@F)

marshal.c: marshal.h Makefile
	$(AM_V_GEN)( echo "#include \"marshal.h\"" ; \
	  $(GLIB_GENMARSHAL) \
	  	--prefix=dawati_netbook_marshal \
		--body \
	  marshal.list --body ) >> xgen-cmc \
	&& cp xgen-cmc marshal.c \
	&& rm -f xgen-cmc

include $(top_srcdir)/build/autotools/Makefile.am.introspec
INTROSPECTION_SCANNER_ARGS += \
	--add-include-path=$(srcdir) \
	--add-include-path=$(MUTTER_GIR_DIR) \
	$(if $(BLUETOOTH_DIR),-L $(BLUETOOTH_DIR),)
INTROSPECTION_COMPILER_ARGS += \
	--includedir=$(srcdir) \
	--includedir=$(MUTTER_TYPELIB_DIR)

BUILT_SOURCES = $(DBUS_GLUE) $(DBUS_BINDINGS) $(MARSHALS) $(ENUMS)

CLEANFILES = $(BUILT_SOURCES) $(STAMPS)
EXTRA_DIST =	marshal.list \
		mnb-panel-dbus.xml \
		mnb-toolbar-dbus.xml \
		mnb-enum-types.h.in \
		mnb-enum-types.c.in \
		$(NULL)

Shell-0.1.gir: libdawati-shell.la $(top_builddir)/st/St-1.0.gir
Shell_0_1_gir_INCLUDES = Clutter-1.0 ClutterX11-1.0 Meta-3.0 Soup-2.4 Mx-2.0 GMenu-3.0
Shell_0_1_gir_CFLAGS = $(libdawati_shell_la_CPPFLAGS) -I $(srcdir) -I $(top_srcdir) -I $(top_builddir)
Shell_0_1_gir_LIBS = libdawati-shell.la
Shell_0_1_gir_FILES = $(libdawati_shell_la_gir_sources)
#Shell_0_1_gir_SCANNERFLAGS = \
	--add-include-path=$(MUTTER_GIR_DIR) \
	$(if $(BLUETOOTH_DIR),-L $(BLUETOOTH_DIR),)
INTROSPECTION_GIRS += Shell-0.1.gir
CLEANFILES += Shell-0.1.gir

ShellJS-0.1.gir: libdawati-shell-js.la
ShellJS_0_1_gir_CFLAGS = $(libdawati_shell_la_CPPFLAGS) -I $(srcdir)
ShellJS_0_1_gir_LIBS = libdawati-shell-js.la
ShellJS_0_1_gir_FILES = $(libdawati_shell_js_la_SOURCES)
INTROSPECTION_GIRS += ShellJS-0.1.gir
CLEANFILES += ShellJS-0.1.gir

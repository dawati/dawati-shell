noinst_LTLIBRARIES = libhal-panel-glib.la

%-bindings.h: %.xml
	dbus-binding-tool --mode=glib-client --output=$@ --prefix=$(subst -,_,$*) $^

BUILT_SOURCES = hal-laptop-panel-bindings.h
libhal_panel_glib_la_CFLAGS = $(DBUS_GLIB_CFLAGS)
libhal_panel_glib_la_LIBADD = $(DBUS_GLIB_LIBS)
libhal_panel_glib_la_SOURCES = $(BUILT_SOURCES) \
			       hal-panel-proxy.h \
			       hal-panel-proxy.c



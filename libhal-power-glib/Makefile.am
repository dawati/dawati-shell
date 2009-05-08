noinst_LTLIBRARIES = libhal-power-glib.la

%-bindings.h: %.xml
	dbus-binding-tool --mode=glib-client --output=$@ --prefix=$(subst -,_,$*) $^

BUILT_SOURCES = hal-system-power-management-bindings.h
libhal_power_glib_la_CFLAGS = $(DBUS_GLIB_CFLAGS)
libhal_power_glib_la_LIBADD = $(DBUS_GLIB_LIBS)
libhal_power_glib_la_SOURCES = $(BUILT_SOURCES) \
			       hal-power-proxy.h \
			       hal-power-proxy.c

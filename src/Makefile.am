
pkglibdir=$(libdir)/@PACKAGE@/plugins/clutter

INCLUDES=@MUTTER_PLUGIN_CFLAGS@

moblin_netbook_la_CFLAGS   = -Werror=missing-prototypes -fPIC
moblin_netbook_la_SOURCES  = moblin-netbook.c
moblin_netbook_la_LDFLAGS  = $(PLUGIN_LT_LDFLAGS) -module -avoid-version -no-undefined
moblin_netbook_la_LIBADD   = @MUTTER_PLUGIN_LIBS@

pkglib_LTLIBRARIES = moblin_netbook.la

# post-install hook to remove the .la and .a files we are not interested in
# (There is no way to stop libtool generating static libs locally, and we
# cannot do this globally because of libmetacity-private.so).
install-exec-hook:
	rm $(DESTDIR)$(pkglibdir)/*.a
	rm $(DESTDIR)$(pkglibdir)/*.la


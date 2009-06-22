m4_define([mutter_version], [2.27.0~0.18])
m4_define([plugin_major_version], [0])
m4_define([plugin_minor_version], [25])
m4_define([plugin_micro_version], [5])
m4_define([plugin_version],
          [plugin_major_version.plugin_minor_version.plugin_micro_version])

m4_define([plugin_api_version], [1.0])

# increase the interface age of 2 for each release
# set to 0 if the API changes
m4_define([plugin_interface_age], [0])
m4_define([plugin_binary_age], [m4_eval(100 * plugin_minor_version + plugin_micro_version)])

m4_define([lt_current], [m4_eval(100 * plugin_minor_version + plugin_micro_version - plugin_interface_age)])
m4_define([lt_revision], [plugin_interface_age])
m4_define([lt_age], [m4_eval(plugin_binary_age - plugin_interface_age)])

AC_PREREQ([2.59])

AC_INIT([mutter-moblin], [plugin_version], [http://moblin.org])
AC_CONFIG_MACRO_DIR([build/autotools])

AC_CONFIG_SRCDIR(src/moblin-netbook.c)

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

CFLAGS="$CFLAGS -Wall -DNBTK_DISABLE_DEPRECATED"

GETTEXT_PACKAGE=mutter-moblin-netbook-plugin
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[Name of default gettext domain])

localedir="$(prefix)/$(DATADIRNAME)/locale"
AC_SUBST(localedir)

IT_PROG_INTLTOOL([0.34.90])
AC_PROG_CC
AM_PROG_CC_C_O
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL
AC_CHECK_FUNCS([localtime_r])

# We have a patch to libgnome-menu that adds an accessor for the
# GenericName desktop entry field.
SAVE_LIBS=${LIBS}
AC_CHECK_LIB([gnome-menu], [gmenu_tree_entry_get_generic_name],
[
  AC_DEFINE([GMENU_WITH_GENERIC_NAME], [1], [libgnome-menu has the GenericName accessor])
])
LIBS=${SAVE_LIBS}

# Make libtool respect the make -s switch (kudos to jacob berman)
changequote(,)dnl
LIBTOOL="${LIBTOOL} \$(shell echo \"\$(MFLAGS)\" | awk '/^[^ ]*s/ { print \"--silent\" }')"
changequote([,])dnl


AM_GLIB_GNU_GETTEXT

# Libmoblin-panel
PKG_CHECK_MODULES(LIBMPL_CLUTTER,
                  dbus-glib-1
                  nbtk-1.0)

LIBMPL_CLUTTER_CFLAGS="${LIBMPL_CLUTTER_CFLAGS} "'-I$(top_srcdir)/src'
AC_SUBST([LIBMPL_CLUTTER_CFLAGS])
AC_SUBST([LIBMPL_CLUTTER_LIBS])

PKG_CHECK_MODULES(LIBMPL_GTK,
                  dbus-glib-1
                  gtk+-2.0)

LIBMPL_GTK_CFLAGS="${LIBMPL_GTK_CFLAGS} "'-I$(top_srcdir)/src'
AC_SUBST([LIBMPL_GTK_CFLAGS])
AC_SUBST([LIBMPL_GTK_LIBS])

# Mutter-moblin
PKG_CHECK_MODULES(MUTTER_PLUGIN,
                  metacity-plugins >= mutter_version dnl
                  nbtk-1.0 >= 0.8.0                  dnl
                  libstartup-notification-1.0        dnl
                  gtk+-2.0                           dnl
                  dbus-glib-1                        dnl
                  mojito-client                      dnl
                  gio-unix-2.0)

MUTTER_PLUGIN_CFLAGS="${MUTTER_PLUGIN_CFLAGS} "'-I$(top_srcdir)/libmoblin-panel'
AC_SUBST([MUTTER_PLUGIN_CFLAGS])

# Penge
PKG_CHECK_MODULES(PENGE,
                  libjana-ecal
                  mojito-client
                  gtk+-2.0 >= 2.14
                  nbtk-1.0
                  gconf-2.0
                  gio-2.0
                  gio-unix-2.0)

# Ahoghill
AC_ARG_ENABLE(ahoghill, AC_HELP_STRING([--enable-ahoghill],[Enable Ahoghill Media Panel]), enable_ahoghill="$enableval", enable_ahoghill=no)

if test "x$enable_ahoghill" = "xyes"; then
	AC_DEFINE(USE_AHOGHILL,1,[Define if we are using Ahoghill])
	PKG_CHECK_MODULES(AHOGHILL, nbtk-1.0 bickley-0.4 >= 0.4.1 gdk-pixbuf-2.0 bognor-regis-0.4 >= 0.4.3 gtk+-2.0)
fi

AM_CONDITIONAL(WITH_AHOGHILL, test x"$enable_ahoghill" = xyes)

# Applications panel
PKG_CHECK_MODULES(APPLICATIONS,
                  clutter-x11-0.9
                  gdk-x11-2.0
                  gio-unix-2.0
                  gtk+-2.0
                  libgnome-menu
                  nbtk-1.0)

APPLICATIONS_CFLAGS="${APPLICATIONS_CFLAGS} ${PENGE_CFLAGS} "\
'-I$(top_srcdir)/libmoblin-panel '\
'-I$(top_srcdir)/penge -I$(top_srcdir)/src '
AC_SUBST([APPLICATIONS_CFLAGS])
APPLICATIONS_LIBS="${APPLICATIONS_LIBS} ${PENGE_LIBS} "\
'$(top_builddir)/libmoblin-panel/moblin-panel/libmoblin-panel-clutter.la '\
'$(top_builddir)/penge/penge/libpenge.la'
AC_SUBST([APPLICATIONS_LIBS])

# Netpanel
AC_ARG_ENABLE([netpanel],
	AC_HELP_STRING([--enable-netpanel],
		[Enable building of the internet panel]),
		[build_netpanel=yes])

if test x"$enable_netpanel" = xyes; then
	AC_DEFINE(WITH_NETPANEL, 1, [Defined when building the netpanel])
	PKG_CHECK_MODULES(NETPANEL, cluttermozembed mhs-1.0)
fi

AM_CONDITIONAL(WITH_NETPANEL, test x"$enable_netpanel" = xyes)

# People panel
AC_ARG_ENABLE([people],
	AC_HELP_STRING([--enable-people],
		[Enable building of the people panel]),
		[enable_people=yes])

if test x"$enable_people" = xyes; then
	AC_DEFINE(WITH_PEOPLE, 1, [Defined when building the people])
	PKG_CHECK_MODULES(PEOPLE, anerley gtk+-2.0 clutter-x11-0.9 gio-2.0 gio-unix-2.0)
fi

AM_CONDITIONAL(WITH_PEOPLE, test x"$enable_people" = xyes)

AC_ARG_ENABLE([debug],
	AC_HELP_STRING([--enable-debug],
		[Enable debugging features, whatever that means]),
		[enable_debug="$enableval"], [enable_debug=no])

if test x"$enable_debug" = xyes; then
   CFLAGS="$CFLAGS -g -O0"
fi

# glib-genmarshal
GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

MUTTER_PLUGIN_DIR=$($PKG_CONFIG metacity-plugins --variable=mutterplugindir)
AC_SUBST(MUTTER_PLUGIN_DIR)

PLUGIN_LT_CURRENT=lt_current
PLUGIN_LT_REV=lt_revision
PLUGIN_AGE=lt_age
PLUGIN_LT_VERSION="$PLUGIN_LT_CURRENT:$PLUGIN_LT_REV:$PLUGIN_LT_AGE"

PLUGIN_LT_LDFLAGS="-version-info $PLUGIN_LT_VERSION"

AC_SUBST(PLUGIN_LT_VERSION)
AC_SUBST(PLUGIN_LT_LDFLAGS)

SHAVE_INIT([build/autotools], [enable])

AC_OUTPUT([
Makefile
applications/Makefile
applications/src/Makefile
applications/tests/Makefile
applications/theme/Makefile
build/Makefile
build/autotools/Makefile
build/autotools/shave
build/autotools/shave-libtool
data/Makefile
src/Makefile
src/tray/Makefile
src/effects/Makefile
po/Makefile.in
tests/Makefile
penge/Makefile
penge/penge/Makefile
penge/examples/Makefile
people/Makefile
people/src/Makefile
ahoghill/Makefile
ahoghill/ahoghill/Makefile
ahoghill/examples/Makefile
libmoblin-panel/Makefile
libmoblin-panel/moblin-panel-gtk.pc
libmoblin-panel/moblin-panel-clutter.pc
libmoblin-panel/moblin-panel/Makefile
libmoblin-panel/tests/Makefile
myzone/Makefile
myzone/src/Makefile
])

echo "
        Mutter Moblin Netbook Plugin version plugin_version
        ===================================================

        Configuration:

            Mutter version   :    mutter_version
            Installation path:    ${MUTTER_PLUGIN_DIR}
	    Debug            :    ${enable_debug} (CFLAGS: ${CFLAGS})

        Type make to build the plugin.
"

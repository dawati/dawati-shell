m4_define([mutter_version], [2.28.1_0.0])
m4_define([plugin_major_version], [0])
m4_define([plugin_minor_version], [51])
m4_define([plugin_micro_version], [0])
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

# defaults
m4_define([default_enable_cache],    [no])

AC_PREREQ([2.59])

AC_INIT([mutter-moblin], [plugin_version], [http://moblin.org])
AC_CONFIG_MACRO_DIR([build/autotools])

AC_CONFIG_SRCDIR(src/moblin-netbook.c)

AM_INIT_AUTOMAKE([dist-bzip2])

PLUGIN_MAJOR_VERSION=plugin_major_version
PLUGIN_MINOR_VERSION=plugin_minor_version
PLUGIN_MICRO_VERSION=plugin_micro_version
PLUGIN_VERSION=plugin_version
PLUGIN_API_VERSION=plugin_api_version
AC_SUBST(PLUGIN_MAJOR_VERSION)
AC_SUBST(PLUGIN_MINOR_VERSION)
AC_SUBST(PLUGIN_MICRO_VERSION)
AC_SUBST(PLUGIN_VERSION)
AC_SUBST(PLUGIN_API_VERSION)

AM_CONFIG_HEADER([config.h])

GETTEXT_PACKAGE=mutter-moblin-netbook-plugin
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],
                   ["$GETTEXT_PACKAGE"],
                   [Name of default gettext domain])

IT_PROG_INTLTOOL([0.34.90])
AS_ALL_LINGUAS

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
               AC_DEFINE([GMENU_WITH_GENERIC_NAME],
                         [1],
                         [libgnome-menu has the GenericName accessor])
             ])
LIBS=${SAVE_LIBS}

AM_GLIB_GNU_GETTEXT

# Libmoblin-panel
PKG_CHECK_MODULES(LIBMPL,
                  dbus-glib-1
                  mx-1.0 = 0.3.0
                  gtk+-2.0
                  gio-unix-2.0)

MUTTER_MOBLIN_THEME_DIR="${datadir}/${PACKAGE}/theme"
AC_SUBST([MUTTER_MOBLIN_THEME_DIR])

MUTTER_MOBLIN_PANELS_DIR="${datadir}/${PACKAGE}/panels"
AC_SUBST([MUTTER_MOBLIN_PANELS_DIR])

AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
  AC_MSG_ERROR([gconftool-2 not found])
fi

AM_GCONF_SOURCE_2

# Mutter-moblin
PKG_CHECK_MODULES(MUTTER_PLUGIN,
                  mutter-plugins >= mutter_version dnl
                  mx-1.0 = 0.3.0                     dnl
                  libstartup-notification-1.0        dnl
                  gtk+-2.0                           dnl
                  dbus-glib-1                        dnl
                  gconf-2.0
                  gio-unix-2.0)

AC_ARG_ENABLE([scaled-background],
	AC_HELP_STRING([--enable-scaled-background],
		[Scale the mutter-moblin background image]),
		[scale_background="$enableval"], [scale_background=no])

if test x"$scale_background" = xyes; then
	AC_DEFINE(USE_SCALED_BACKGROUND, 1, [Use scaled background])
fi

AC_ARG_ENABLE([power-applet],
	AC_HELP_STRING([--disable-power-applet],
		[Disable power applet]),
		[power_applet="$enableval"], [power_applet=yes])

if test x"$power_applet" = xno; then
	AC_DEFINE(DISABLE_POWER_APPLET, 1, [Disable power applet])
fi

AC_ARG_ENABLE([cache],
              [AC_HELP_STRING([--enable-cache],
                              [Enable Mx image cache generation])],
              [],
              [enable_cache=no])

AS_IF([test "x$enable_cache" = "xyes"],
      [
      AC_CHECK_PROG([MX_CREATE_IMAGE_CACHE],
	        [mx-create-image-cache],
		[$(which mx-create-image-cache)])
])

AM_CONDITIONAL([ENABLE_CACHE],   [test "x$enable_cache" = "xyes"])

AC_ARG_ENABLE([debug],
              [AC_HELP_STRING([--enable-debug],
                              [Enable debugging features, whatever that means])],
              [],
              [enable_debug=no])

CFLAGS="$CFLAGS -Wall"

AS_IF([test "x$enable_debug" = "xyes"], [CFLAGS="$CFLAGS -g -O0"])

# glib-genmarshal
GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)
GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

MUTTER_PLUGIN_DIR=$($PKG_CONFIG mutter-plugins --variable=plugindir)
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
build/Makefile
build/autotools/Makefile
build/autotools/shave
build/autotools/shave-libtool
data/Makefile
src/Makefile
src/effects/Makefile
src/switcher/Makefile
src/notifications/Makefile
po/Makefile.in
tests/Makefile
libmoblin-panel/Makefile
libmoblin-panel/moblin-panel.pc
libmoblin-panel/moblin-panel/Makefile
libmoblin-panel/moblin-panel/mpl-version.h
libmoblin-panel/theme/Makefile
libmoblin-panel/tests/Makefile
])

echo "
        Mutter Moblin Netbook Plugin version plugin_version
        ===================================================

        Configuration:

            Mutter version   :    mutter_version
            Installation path:    ${MUTTER_PLUGIN_DIR}
	    Debug            :    ${enable_debug} (CFLAGS: ${CFLAGS})
	    Scaled background:    ${scale_background}
	    Power Applet     :    ${power_applet}

        Type make to build the plugin.
"

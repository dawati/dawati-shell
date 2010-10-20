m4_define([mutter_version], [2.31.5_1.0])
m4_define([plugin_major_version], [0])
m4_define([plugin_minor_version], [77])
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

AC_INIT([mutter-meego], [plugin_version], [http://meego.org])
AC_CONFIG_MACRO_DIR([build/autotools])

AC_CONFIG_SRCDIR(src/meego-netbook.c)

AM_INIT_AUTOMAKE([tar-ustar dist-bzip2])

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

GETTEXT_PACKAGE=mutter-meego-netbook-plugin
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

# Libmeego-panel
PKG_CHECK_MODULES(LIBMPL,
                  dbus-glib-1
                  mx-1.0 >= 0.9.0
                  gtk+-2.0
                  gio-unix-2.0)

MUTTER_MEEGO_THEME_DIR="${datadir}/${PACKAGE}/theme"
AC_SUBST([MUTTER_MEEGO_THEME_DIR])

MUTTER_MEEGO_PANELS_DIR="${datadir}/${PACKAGE}/panels"
AC_SUBST([MUTTER_MEEGO_PANELS_DIR])

AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
  AC_MSG_ERROR([gconftool-2 not found])
fi

AM_GCONF_SOURCE_2

# Mutter-meego
PKG_CHECK_MODULES(MUTTER_PLUGIN,
                  mutter-plugins >= mutter_version dnl
                  mx-1.0 >= 0.5.0                     dnl
                  libstartup-notification-1.0        dnl
                  gtk+-2.0                           dnl
                  dbus-glib-1                        dnl
                  gconf-2.0
                  gio-unix-2.0
		  xrandr
		  xscrnsaver)

AC_ARG_ENABLE([scaled-background],
	AC_HELP_STRING([--enable-scaled-background],
		[Scale the mutter-meego background image]),
		[scale_background="$enableval"], [scale_background=no])

if test x"$scale_background" = xyes; then
	AC_DEFINE(USE_SCALED_BACKGROUND, 1, [Use scaled background])
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

AC_ARG_ENABLE([profile],
              [AC_HELP_STRING([--enable-profile],
                              [Enable profiling])],
              [],
              [enable_profile=no])

CFLAGS="$CFLAGS -Wall"

AS_IF([test "x$enable_debug" = "xyes"], [CFLAGS="$CFLAGS -g -O0"])
AS_IF([test "x$enable_profile" = "xyes"], [CFLAGS="$CFLAGS -pg"])

# control center capplet
AC_ARG_ENABLE([capplet],
              [AC_HELP_STRING([--disable-capplet],
                              [Disable control center capplet])],
              [],
              [enable_capplet=yes])

if test "$enable_capplet" != "no"; then
  PKG_CHECK_MODULES(CAPPLET, mx-1.0 gconf-2.0)
  PKG_CHECK_MODULES(SYSTRAY, mx-gtk-1.0)
  PKG_CHECK_MODULES(CC, gmodule-export-2.0 libgnome-control-center-extension clutter-gtk-0.10)
  CC_DIR=$($PKG_CONFIG libgnome-control-center-extension --variable=extensiondir)
fi
AC_SUBST(CC_DIR)
AM_CONDITIONAL([HAVE_CAPPLET], [test "$enable_capplet" != "no"])

GTK_DOC_CHECK([1.11])

# we don't want to build the documentation from a Git clone unless we
# explicitly tell configure to do so; this allows avoiding to recurse into
# the documentation directory when building inside Poky for a
# target device that doesn't have gtk-doc installed.
#
# we use autogen.sh as it exists only inside the Git clones, and it
# is not packaged into tarballs.
AM_CONDITIONAL([BUILD_GTK_DOC], [test "x$enable_gtk_doc" = "xyes" || test ! -f "autogen.sh"])

# prefixes for fixing gtk-doc references
CLUTTER_PREFIX="`$PKG_CONFIG --variable=prefix clutter-1.0`"
GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
MX_PREFIX="`$PKG_CONFIG --variable=prefix mx-1.0`"
AC_SUBST(CLUTTER_PREFIX)
AC_SUBST(GLIB_PREFIX)
AC_SUBST(MX_PREFIX)

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
capplet/Makefile
capplet/data/Makefile
capplet/src/Makefile
data/Makefile
src/Makefile
src/effects/Makefile
src/alttab/Makefile
src/notifications/Makefile
src/presence/Makefile
po/Makefile.in
tests/Makefile
libmeego-panel/Makefile
libmeego-panel/docs/Makefile
libmeego-panel/docs/reference/Makefile
libmeego-panel/docs/reference/libmeego-panel/Makefile
libmeego-panel/docs/reference/libmeego-panel/meego-panel-docs.xml
libmeego-panel/meego-panel.pc
libmeego-panel/meego-panel/Makefile
libmeego-panel/meego-panel/mpl-version.h
libmeego-panel/theme/Makefile
libmeego-panel/tests/Makefile
])

echo "
        Mutter Meego Netbook Plugin version plugin_version
        ===================================================

        Configuration:

            Mutter version   :    mutter_version
            Installation path:    ${MUTTER_PLUGIN_DIR}
	    Debug            :    ${enable_debug}
	    Profile          :    ${enable_profile}
	    Scaled background:    ${scale_background}
            API Reference    :    ${enable_gtk_doc}

	    CFLAGS: ${CFLAGS}

        Type make to build the plugin.
"

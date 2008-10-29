
m4_define([mutter_version], [2.25.2])
m4_define([plugin_major_version], [0])
m4_define([plugin_minor_version], [1])
m4_define([plugin_micro_version], [0])
m4_define([plugin_version],
          [plugin_major_version.plugin_minor_version.plugin_micro_version])

AC_PREREQ(2.50)

AC_INIT([mutter-moblin-netbook-plugin], [plugin_version], [http://moblin.org])

AC_CONFIG_SRCDIR(src/moblin-netbook.c)

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

GETTEXT_PACKAGE=mutter-moblin-netbook-plugin
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[Name of default gettext domain])

IT_PROG_INTLTOOL([0.34.90])
AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL

AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES(MUTTER_PLUGIN, metacity-plugins >= mutter_version)

PLUGIN_LT_LDFLAGS="-version-info $PLUGIN_MAJOR_VERSION:$PLUGIN_MINOR_VERSION:$PLUGIN_MICRO_VERSION"
AC_SUBST(PLUGIN_LT_LDFLAGS)

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])

echo "
        Mutter Moblin Netbook Plugin
        ============================

        Configuration:

            Mutter version   :    mutter_version
            Installation path:    ${prefix}


        Type make to build the plugin.
"
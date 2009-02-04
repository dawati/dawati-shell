
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

# Make libtool respect the make -s switch (kudos to jacob berman)
changequote(,)dnl
LIBTOOL="${LIBTOOL} \$(shell echo \"\$(MFLAGS)\" | awk '/^[^ ]*s/ { print \"--silent\" }')"
changequote([,])dnl


AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES(MUTTER_PLUGIN, metacity-plugins >= mutter_version nbtk-1.0 libstartup-notification-1.0 gtk+-2.0 libgnome-menu dbus-glib-1)

PKG_CHECK_MODULES(PENGE, libjana-ecal mojito-client gtk+-2.0 nbtk-1.0)

# glib-genmarshal
GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

PKG_CHECK_MODULES(GTK, gtk+-2.0)

MUTTER_PLUGIN_DIR=$($PKG_CONFIG metacity-plugins --variable=mutterplugindir)
AC_SUBST(MUTTER_PLUGIN_DIR)

PLUGIN_LT_LDFLAGS="-version-info plugin_major_version:plugin_minor_version:plugin_micro_version"
AC_SUBST(PLUGIN_LT_LDFLAGS)

AC_OUTPUT([
Makefile
data/Makefile
src/Makefile
src/tray/Makefile
po/Makefile.in
tests/Makefile
penge/Makefile
penge/penge/Makefile
penge/examples/Makefile
])

echo "
        Mutter Moblin Netbook Plugin version plugin_version
        ===================================================

        Configuration:

            Mutter version   :    mutter_version
            Installation path:    ${MUTTER_PLUGIN_DIR}


        Type make to build the plugin.
"

AC_PREREQ(2.50)

AC_INIT([mutter-moblin-netbook-plugin], [0.0], [put some contact info here])

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

PKG_CHECK_MODULES(MUTTER_PLUGIN, metacity-plugins >= 2.25.2)

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])


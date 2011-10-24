AC_PREREQ(2.53)
AC_INIT(anerley, 0.4.0)
AM_INIT_AUTOMAKE(1.11)
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER(config.h)

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CANONICAL_HOST
AS_ALL_LINGUAS
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=anerley
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],
                   ["$GETTEXT_PACKAGE"],
                   [Gettext domain name])
AM_GLIB_GNU_GETTEXT
GLIB_DEFINE_LOCALEDIR(LOCALEDIR)

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_CC_C_O

CFLAGS="$CFLAGS -Wall"

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.14 gobject-2.0)
PKG_CHECK_MODULES(TELEPATHY, telepathy-glib >= 0.9.0)
PKG_CHECK_MODULES(MX, mx-1.0)
PKG_CHECK_MODULES(EBOOK, libebook-1.2)
PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1)

AC_MSG_CHECKING([how to detect we are online])
AC_ARG_WITH([online],
        [AS_HELP_STRING([--with-online],
                [how to check if we are online (always, connman, networkmanager, test)])],
        [], [with_online=always])
AS_IF(
        [test "$with_online" = always],
        [
        AC_MSG_RESULT([always])
        AC_DEFINE([WITH_ONLINE_ALWAYS], 1, [No online detection])
        ],

        [test "$with_online" = connman],
        [
        AC_MSG_RESULT([Connection Manager])
        AC_DEFINE([WITH_ONLINE_CONNMAN], 1, [ConnMan online detection])
        ],

        [test "$with_online" = networkmanager],
        [
        AC_MSG_RESULT([Network Manager])
        PKG_CHECK_MODULES(ONLINE, libnm_glib >= 0.7)
        AC_DEFINE([WITH_ONLINE_NM], 1, [NM online detection])
        ],

        [AC_MSG_ERROR([Unknown argument to --with-online])]
)

AC_OUTPUT([
        Makefile
        anerley/Makefile
        src/Makefile
        tests/Makefile
        data/Makefile
        anerley.pc
        po/Makefile.in
])

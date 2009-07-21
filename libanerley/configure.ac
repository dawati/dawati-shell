AC_PREREQ(2.53)
AC_INIT(anerley, 0.0.17)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=anerley
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],
                   ["$GETTEXT_PACKAGE"],
                   [Gettext domain name])
AM_GLIB_GNU_GETTEXT

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_CC_C_O

CFLAGS="$CFLAGS -Wall"

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.14 gobject-2.0)
PKG_CHECK_MODULES(TELEPATHY, telepathy-glib)
PKG_CHECK_MODULES(MC, libmissioncontrol)
PKG_CHECK_MODULES(NBTK, nbtk-1.0 >= 0.15.0)
PKG_CHECK_MODULES(EBOOK, libebook-1.2)

AC_OUTPUT([
        Makefile
        anerley/Makefile
        tests/Makefile
        data/Makefile
        anerley.pc
        po/Makefile.in
])

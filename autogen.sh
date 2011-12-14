#!/bin/bash
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="dawati-shell"
REQUIRED_AUTOMAKE_VERSION=1.10

(test -f $srcdir/configure.ac \
  && test -d $srcdir/src) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gnome-autogen.sh || {
    echo "You need to install gnome-common from GNOME Subversion (or from"
    echo "your distribution's package manager)."
    exit 1
}

GTKDOCIZE=`which gtkdocize`
if test -z $GTKDOCIZE; then
        echo "*** No gtk-doc support ***"
        echo "EXTRA_DIST =" > gtk-doc.make
else
        gtkdocize || exit $?
        # we need to patch gtk-doc.make to support pretty output with
        # libtool 1.x.  Should be fixed in the next version of gtk-doc.
        # To be more resilient with the various versions of gtk-doc one
        # can find, just sed gkt-doc.make rather than patch it.
        sed -e 's#) --mode=compile#) --tag=CC --mode=compile#' gtk-doc.make > gtk-doc.temp \
                && mv gtk-doc.temp gtk-doc.make
        sed -e 's#) --mode=link#) --tag=CC --mode=link#' gtk-doc.make > gtk-doc.temp \
                && mv gtk-doc.temp gtk-doc.make
fi

USE_GNOME2_MACROS=1 USE_COMMON_DOC_BUILD=yes . gnome-autogen.sh

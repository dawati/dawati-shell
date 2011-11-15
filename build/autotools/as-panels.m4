dnl This is derived from GStreamers common/m4/gst-args.m4
dnl DAWATI_PANELS_ALL/DAWATI_PANELS_SELECTED contain all/selected panels

dnl sets WITH_PANELS to the list of plug-ins given as an argument
dnl also clears DAWATI_PANELS_ALL and DAWATI_PANELS_SELECTED
AC_DEFUN([AS_DAWATI_ARG_WITH_PANELS],
[
  AC_ARG_WITH(panels,
    AC_HELP_STRING([--with-panels],
      [comma-separated list of panels to compile]),
    [WITH_PANELS=$withval])

  DAWATI_PANELS_ALL=""
  DAWATI_PANELS_SELECTED=""

  AC_SUBST(DAWATI_PANELS_ALL)
  AC_SUBST(DAWATI_PANELS_SELECTED)
])

dnl AG_DAWATI_PANEL(PANEL-NAME)
dnl
dnl This macro adds the plug-in <PANEL-NAME> to DAWATI_PANELS_ALL. Then it
dnl checks if WITH_PANELS is empty or the panel is present in WITH_PANELS,
dnl and if so adds it to DAWATI_PANELS_SELECTED. Then it checks if the panel
dnl is present in WITHOUT_PANELS (ie. was disabled specifically) and if so
dnl removes it from DAWATI_PANELS_SELECTED.
dnl
dnl The macro will call AM_CONDITIONAL(USE_PANEL_<PANEL-NAME>, ...) to allow
dnl control of what is built in Makefile.ams.
AC_DEFUN([AS_DAWATI_PANEL],
[
  DAWATI_PANELS_ALL="$DAWATI_PANELS_ALL [$1]"

  define([pname_def],translit([$1], -a-z, _a-z))
  define([PNAME_DEF],translit(PANEL_[$1], -a-z, _A-Z))

  AC_ARG_ENABLE([$1],
    AC_HELP_STRING([--disable-[$1]], [disable $1 panel]),
    [
      case "${enableval}" in
        yes) [dawati_use_]pname_def[_panel]=yes ;;
        no) [dawati_use_]pname_def[_panel]=no ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-$1]) ;;
       esac
    ])

  if test x$[dawati_use_]pname_def[_panel] = xyes; then
    AC_MSG_NOTICE(enabling panel $1)
    WITH_PANELS="$WITH_PANELS [$1]"
  fi
  if test x$[dawati_use_]pname_def[_panel] = xno; then
    AC_MSG_NOTICE(disabling panel $1)
    WITHOUT_PANELS="$WITHOUT_PANELS [$1]"
  fi

  if [[ -z "$WITH_PANELS" ]] || echo " [$WITH_PANELS] " | tr , ' ' | grep -i " [$1] " > /dev/null; then
    if test "x$2" != x ; then
      PKG_CHECK_MODULES(PNAME_DEF, [$2])
    fi
    if test "x$3" != x ; then
      AC_MSG_NOTICE([])
      $3
    fi
    dnl If we haven't set it yet, it's time to export the variable that
    dnl can be use to tell if we want to build this panel
    if test -z "$[dawati_use_]pname_def[_panel]"; then
      [dawati_use_]pname_def[_panel]=yes
    fi
    DAWATI_PANELS_SELECTED="$DAWATI_PANELS_SELECTED [$1]"
  else
    if test "x$4" != x ; then
      AC_MSG_NOTICE([])
      $4
    fi
  fi
  if echo " [$WITHOUT_PANELS] " | tr , ' ' | grep -i " [$1] " > /dev/null; then
    DAWATI_PANELS_SELECTED=`echo " $DAWATI_PANELS_SELECTED " | $SED -e 's/ [$1] / /'`
  fi
  AM_CONDITIONAL([USE_]PNAME_DEF, echo " $DAWATI_PANELS_SELECTED " | grep -i " [$1] " > /dev/null)

  dnl datadir
  AC_SUBST(PNAME_DEF[_LIBS],
           [$PNAME_DEF[_LIBS]' $(top_builddir)/libdawati-panel/dawati-panel/libdawati-panel.la'])
  AC_SUBST(PNAME_DEF[_THEMEDIR],'$(MUTTER_DAWATI_THEME_DIR)/$1')
  AC_SUBST(PNAME_DEF[_DATADIR],'$(pkgdatadir)/$1')

  dnl additionnal cflags/libs (themedir, localedir, datadir, pkgdatadir)
  AC_SUBST(PNAME_DEF[_CFLAGS],
           [$PNAME_DEF[_CFLAGS]' $(LIBMPL_CFLAGS) -I$(top_builddir) -I$(top_srcdir)/libdawati-panel -DDATADIR=\"$(datadir)\" -DPKGDATADIR=\"$(pkgdatadir)/$1\" -DLOCALEDIR=\"$(localedir)\" -DTHEMEDIR=\"$(MUTTER_DAWATI_THEME_DIR)/$1\"'])
  AC_SUBST(PNAME_DEF[_LIBS],
           [$PNAME_DEF[_LIBS]' $(LIBMPL_LIBS) $(top_builddir)/libdawati-panel/dawati-panel/libdawati-panel.la'])

  dnl theming definitions
  dnl AC_DEFINE([[THEMEDIR_]PNAME_DEF],THEMEDIR "[panel-]pname_def",[Theme directory for [panel-]pname_def])

  undefine([pname_def])
  undefine([PNAME_DEF])
])

dnl AG_DAWATI_DISABLE_PANEL(PANEL-NAME)
dnl
dnl This macro disables the plug-in <PANEL-NAME> by removing it from
dnl DAWATI_PANELS_SELECTED.
AC_DEFUN([AS_DAWATI_DISABLE_PANEL],
[
  DAWATI_PANELS_SELECTED=`echo " $DAWATI_PANELS_SELECTED " | $SED -e 's/ [$1] / /'`
  AM_CONDITIONAL([USE_PANEL_]translit([$1], a-z, A-Z), false)
])

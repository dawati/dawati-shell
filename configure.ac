m4_define([plugin_major_version], [0])
m4_define([plugin_minor_version], [90])
m4_define([plugin_micro_version], [13])
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

AC_INIT([dawati-shell], [plugin_version], [http://dawati.org])
AC_CONFIG_MACRO_DIR([build/autotools])
AC_CONFIG_AUX_DIR([build])

AC_CONFIG_SRCDIR(shell/dawati-netbook.c)

AM_INIT_AUTOMAKE([foreign -Wno-portability tar-ustar no-dist-gzip dist-xz])

DAWATI_PANEL_MAJOR_VERSION=plugin_major_version
DAWATI_PANEL_MINOR_VERSION=plugin_minor_version
DAWATI_PANEL_MICRO_VERSION=plugin_micro_version
DAWATI_PANEL_VERSION=plugin_version
AC_SUBST(DAWATI_PANEL_MAJOR_VERSION)
AC_SUBST(DAWATI_PANEL_MINOR_VERSION)
AC_SUBST(DAWATI_PANEL_MICRO_VERSION)
AC_SUBST(DAWATI_PANEL_VERSION)

AM_CONFIG_HEADER([config.h])

GETTEXT_PACKAGE=dawati-shell
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],
                   ["$GETTEXT_PACKAGE"],
                   [Name of default gettext domain])

# enable quiet ("silent") builds, if available
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

IT_PROG_INTLTOOL([0.34.90])
AS_ALL_LINGUAS

AC_PROG_CC
AC_PROG_CXX
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


# Dependencies
GDK_DEPS="gdk-x11-3.0"
GTK_DEPS="gtk+-3.0"
CLUTTER_DEPS="clutter-1.0 >= 1.8"
MX_DEPS="mx-2.0"
CLUTTER_GTK_DEPS="clutter-gtk-1.0"
MX_GTK_DEPS="mx-gtk-1.0 >= 1.4.0"
ZEITGEIST_DEPS="zeitgeist-1.0 >= 0.3.12"

GLIB_GSETTINGS

# Libdawati-panel
PKG_CHECK_MODULES(LIBMPL,
                  gio-unix-2.0
                  dbus-glib-1
                  $GTK_DEPS
                  $MX_DEPS)

AC_ARG_WITH([style],
            [AS_HELP_STRING([--with-style], [])],
            [style=$withval],
            [style=$PACKAGE])

DAWATI_RUNTIME_THEME_DIR="${datadir}/${style}/theme"
AC_SUBST([DAWATI_RUNTIME_THEME_DIR])

DAWATI_RUNTIME_STYLE_DIR="${datadir}/${style}/style"
AC_SUBST([DAWATI_RUNTIME_STYLE_DIR])

DAWATI_THEME_DIR="${datadir}/${PACKAGE}/theme"
AC_SUBST([DAWATI_THEME_DIR])

DAWATI_PANELS_DIR="${datadir}/${PACKAGE}/panels"
AC_SUBST([DAWATI_PANELS_DIR])

AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
  AC_MSG_ERROR([gconftool-2 not found])
fi

AM_GCONF_SOURCE_2

# Mutter-dawati
AC_ARG_ENABLE([mutter],
              [AC_HELP_STRING([--disable-mutter],
                              [Disable mutter])],
              [],
              [enable_mutter=yes])

if test "$enable_mutter" != "no"; then
PKG_CHECK_MODULES(MUTTER_PLUGIN,
                  [gio-unix-2.0
                   dbus-glib-1
                   gconf-2.0
                   mutter-plugins >= 3.2
                   libmutter >= 3.2
                   libstartup-notification-1.0
                   $GTK_DEPS
                   $MX_DEPS
		   xrandr
		   xscrnsaver])
fi

AM_CONDITIONAL([HAVE_MUTTER], [test "$enable_mutter" != "no"])

AC_ARG_ENABLE([scaled-background],
	AC_HELP_STRING([--enable-scaled-background],
		[Scale the mutter-dawati background image]),
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

AM_CONDITIONAL([ENABLE_CACHE],   [test "$enable_cache" = "yes"])

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

AS_IF([test "x$enable_debug" = "xyes"], [CFLAGS="$CFLAGS -g3 -O0"])
AS_IF([test "x$enable_profile" = "xyes"], [CFLAGS="$CFLAGS -pg"])

# zeitgeist
AC_ARG_ENABLE([zeitgeist],
              [AC_HELP_STRING([--enable-zeitgeist],
                              [Enable zeitgeist features])],
              [1],
              [enable_zeitgeist=no])

if test x"$enable_zeitgeist" = xyes; then
	AC_DEFINE(WITH_ZEITGEIST, 1, [Use Zeitgeist features])
fi

# control center capplet
AC_ARG_ENABLE([capplet],
              [AC_HELP_STRING([--enable-capplet],
                              [Enable control center capplet])],
              [],
              [enable_capplet=no])

if test "$enable_capplet" != "no"; then
  PKG_CHECK_MODULES(CAPPLET, [gio-unix-2.0
                              gconf-2.0
                              gmodule-export-2.0
                              $CLUTTER_GTK_DEPS
                              $MX_DEPS
                              $MX_GTK_DEPS
                              libgnome-control-center-extension])
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
AM_CONDITIONAL([BUILD_GTK_DOC], [test "$enable_gtk_doc" = "yes" || test ! -f "autogen.sh"])

# GObject-Introspection check

GOBJECT_INTROSPECTION_CHECK([gi_req_version])

# prefixes for fixing gtk-doc references
CLUTTER_PREFIX="`$PKG_CONFIG --variable=prefix $CLUTTER_DEPS`"
GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
MX_PREFIX="`$PKG_CONFIG --variable=prefix $MX_DEPS`"
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

GJS_CONSOLE=`$PKG_CONFIG --variable=gjs_console gjs-1.0`
AC_SUBST(GJS_CONSOLE)

###########################
# Available panels
###########################

# Default list of panels to compile
WITH_PANELS="applications,datetime,devices,music,myzone,networks,people,power-icon,status,web,switcher,bluetooth"

# Provide a --with-panels/--without-panels configure option
AS_DAWATI_ARG_WITH_PANELS

# Applications
AS_DAWATI_PANEL(applications, [clutter-x11-1.0
                               gio-unix-2.0
                               gthread-2.0
                               gconf-2.0
                               $GTK_DEPS
                               $MX_DEPS
                               libgnome-menu-3.0
                               libwnck-3.0])
# Bluetooth
AS_DAWATI_PANEL(bluetooth, [$MX_DEPS
                            libnotify
                            gnome-bluetooth-1.0])
BLUETOOTH_APPLET_LIBS=`$PKG_CONFIG --variable=applet_libs gnome-bluetooth-1.0`
AC_SUBST(BLUETOOTH_APPLET_LIBS)

# Date & time
AS_DAWATI_PANEL(datetime, [glib-2.0
                           gthread-2.0
                           gconf-2.0
                           libxml-2.0
                           $CLUTTER_DEPS
                           $MX_DEPS
                           gweather-3.0
                           libnotify
                           libcanberra
                           libjana-ecal
                           geoclue])

# Devices
AS_DAWATI_PANEL(devices, [glib-2.0
                          gthread-2.0
                          gobject-2.0
                          upower-glib
                          gconf-2.0
                          $CLUTTER_GTK_DEPS
                          xrandr
                          xext
                          libpulse-mainloop-glib >= 0.9.15,
                          libnotify])

# Music
AS_DAWATI_PANEL(music, [])

# Myzone
AS_DAWATI_PANEL(myzone, [gthread-2.0
                         gconf-2.0
                         $GDK_DEPS])

# Networks
AS_DAWATI_PANEL(networks, [gconf-2.0
                           gthread-2.0
                           dbus-glib-1 >= 0.73
                           $GTK_DEPS
                           rest-0.7])

if test x$dawati_use_networks_panel = xyes  || \
   test x$dawati_use_bluetooth_panel = xyes || \
   test x$dawati_use_devices_panel = xyes; then
   PKG_CHECK_MODULES(CARRICK, [gthread-2.0
                               gconf-2.0
                               dbus-glib-1
                               $GTK_DEPS
                               $MX_DEPS
                               libnotify])
   PKG_CHECK_MODULES(CALLOUTS, [gthread-2.0
                                gconf-2.0
                                dbus-glib-1
                                $GTK_DEPS
                                rest-0.7])

   # Networks: openconnect
   AC_MSG_CHECKING([whether to enable openconnect support])
   AC_ARG_WITH([openconnect],[AS_HELP_STRING([--without-openconnect], [disable support for OpenConnect VPN])],
               [], [with_openconnect=yes])
   AS_IF([test "$with_openconnect" = yes],
         [
            AC_MSG_RESULT([yes])
            AC_DEFINE([WITH_OPENCONNECT], 1, [Use OpenConnect])
            PKG_CHECK_MODULES(OPENCONNECT, openconnect)
         ],
         AC_MSG_RESULT([no]))

   # Networks: mobile provider db
   AC_MSG_CHECKING([for mobile-broadband-provider-info])
   PKG_CHECK_EXISTS(mobile-broadband-provider-info,
                    _PKG_CONFIG(MOBILE_DATA, [variable=database], [mobile-broadband-provider-info])
                    AC_DEFINE_UNQUOTED(MOBILE_DATA, "$pkg_cv_MOBILE_DATA", [Mobile provider database])
                    AC_MSG_RESULT([yes]),
                    AC_MSG_ERROR([Cannot find mobile-broadband-provider-info]))

   # Networks: ISO codes
   AC_MSG_CHECKING([for iso-codes])
   PKG_CHECK_EXISTS(iso-codes,
                    _PKG_CONFIG(ISOCODES, [variable=prefix], [iso-codes])
                    AC_DEFINE_UNQUOTED(ISOCODES_PREFIX, "$pkg_cv_ISOCODES", [iso-codes prefix])
                    AC_MSG_RESULT([yes]),
                    AC_MSG_ERROR([Cannot find iso-codes]))

fi
AM_CONDITIONAL([USE_CARRICK_CALLOUTS], [test "$dawati_use_networks_panel" = "yes" || test "$dawati_use_bluetooth_panel" = "yes"])
AM_CONDITIONAL([WITH_OPENCONNECT], [test "$with_openconnect" = "yes"])


# People
AS_DAWATI_PANEL(people, [gio-2.0
                         gio-unix-2.0
                         gthread-2.0
                         telepathy-glib >= 0.16.0
                         folks >= 0.6.6
                         folks-telepathy >= 0.6.6
                         libebook-1.2
                         dbus-glib-1
                         $GTK_DEPS
                         $MX_DEPS])

# Power
AS_DAWATI_PANEL(power-icon, [gconf-2.0
                             gthread-2.0
                             $GDK_DEPS
                             $CLUTTER_DEPS
                             $CLUTTER_GTK_DEPS
                             $MX_DEPS
                             upower-glib
                             libnotify])

# Status
AS_DAWATI_PANEL(status, [dbus-glib-1
                         gthread-2.0
                         $CLUTTER_DEPS
                         $CLUTTER_GTK_DEPS
                         $MX_DEPS
                         libsocialweb-client >= 0.25.3
                         champlain-0.12
                         geoclue
                         gconf-2.0])

# Web
AS_DAWATI_PANEL(web, [sqlite3
                      gthread-2.0
                      $GTK_DEPS
                      $CLUTTER_DEPS
                      $MX_DEPS])

# Switcher
AS_DAWATI_PANEL(switcher, [gthread-2.0
                           $GTK_DEPS
                           $CLUTTER_GTK_DEPS
                           $MX_DEPS
                           libwnck-3.0])


###########################
# Cross panel options
###########################

if test x$dawati_use_myzone_panel = xyes || test x$dawati_use_datetime_panel = xyes; then
   PKG_CHECK_MODULES(PENGE, [gio-2.0
                             gio-unix-2.0
                             gthread-2.0
                             gconf-2.0
                             libjana-ecal
                             libsocialweb-client
                             telepathy-glib >= 0.14
                             $GTK_DEPS
                             $MX_DEPS
			     $ZEITGEIST_DEPS])

   PKG_CHECK_MODULES(MAILME, [glib-2.0
                              gthread-2.0
                              gobject-2.0
                              gio-2.0
                              dbus-glib-1
                              telepathy-glib >= 0.14])
fi
AM_CONDITIONAL([USE_PENGE_MAILME],
               [test "$dawati_use_myzone_panel" = "yes" || test "$dawati_use_datetime_panel" = "yes"])

if test x$dawati_use_devices_panel = xyes || test x$dawati_use_power_icon_panel = xyes; then
   #
   # Egg Console Kit
   #
   PKG_CHECK_MODULES(ECK, dbus-glib-1)

   #
   # Gnome Power Manager
   #
   PKG_CHECK_MODULES(GPM, [glib-2.0
                           gobject-2.0
                           $GDK_DEPS
                           xrandr
                           xext])

   #
   # Gnome Volume Control
   #
   PKG_CHECK_MODULES(GVC, libpulse-mainloop-glib >= 0.9.15)

fi
AM_CONDITIONAL([USE_ECK_GPM_GVC],
               [test "$dawati_use_devices_panel" = "yes" || test "$dawati_use_power_icon_panel" = "yes"])

if test x$dawati_use_people_panel = xyes || test x$dawati_use_status_panel = xyes; then
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
        PKG_CHECK_MODULES(ONLINE, libnm-glib >= 0.7)
        AC_DEFINE([WITH_ONLINE_NM], 1, [NM online detection])
        ],

        [AC_MSG_ERROR([Unknown argument to --with-online])])
fi

# zeitgeist
PKG_CHECK_MODULES(ZEITGEIST, zeitgeist-1.0)

# libnotify API breakage upon 0.7
PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= 0.7,
                  AC_DEFINE(HAVE_NOTIFY_0_7, 1, [we have libnotify 0.7 or better]),
                  AC_MSG_NOTICE([]))

# Desktop files for panels
AM_CONDITIONAL([ENABLE_DAWATI_DESKTOP_FILE], [test "$enable_mdesktop" = "yes"])

# Capplets
AC_ARG_ENABLE([capplet],
              [AC_HELP_STRING([--disable-capplet],
                              [Disable the building of the capplet])],
              [enable_cappled=$enableval],
              [enable_capplet=yes])

AM_CONDITIONAL([ENABLE_CAPPLET], [test "$enable_capplet" = "yes"])

# Test tools
AC_MSG_CHECKING([whether to install test tools])
AC_ARG_ENABLE([test-tools],
              [AC_HELP_STRING([--enable-test-tools],
                              [Enable extra tools for testing and debugging])])
AC_MSG_RESULT([$enable_test_tools])
AM_CONDITIONAL([INSTALL_TEST_TOOLS], [test "$enable_test_tools" = "yes"])

#
AC_OUTPUT([
Makefile
build/Makefile
build/autotools/Makefile
capplet/Makefile
capplet/data/Makefile
capplet/src/Makefile

data/Makefile
data/theme/Makefile
data/theme/applications/Makefile
data/theme/bluetooth/Makefile
data/theme/common/Makefile
data/theme/datetime/Makefile
data/theme/devices/Makefile
data/theme/devices/icons/Makefile
data/theme/music/Makefile
data/theme/myzone/Makefile
data/theme/networks/Makefile
data/theme/panel/Makefile
data/theme/people/Makefile
data/theme/power-icon/Makefile
data/theme/shared/Makefile
data/theme/status/Makefile
data/theme/switcher/Makefile
data/theme/web/Makefile
data/theme/zones/Makefile

shell/Makefile
shell/effects/Makefile
shell/alttab/Makefile
shell/notifications/Makefile
shell/presence/Makefile
po/Makefile.in
tests/Makefile

libdawati-panel/Makefile
libdawati-panel/docs/Makefile
libdawati-panel/docs/reference/Makefile
libdawati-panel/docs/reference/libdawati-panel/Makefile
libdawati-panel/docs/reference/libdawati-panel/dawati-panel-docs.xml
libdawati-panel/dawati-panel.pc
libdawati-panel/dawati-panel/Makefile
libdawati-panel/dawati-panel/mpl-version.h
libdawati-panel/js/Makefile
libdawati-panel/tests/Makefile

carrick/Makefile

callouts/3g/Makefile
callouts/Makefile
callouts/openconnect/Makefile

egg-console-kit/Makefile

gpm/Makefile

gvc/Makefile

mailme/Makefile

penge/Makefile

panels/Makefile

panels/applications/Makefile
panels/applications/src/Makefile
panels/applications/tests/Makefile
panels/applications/data/Makefile

panels/datetime/Makefile
panels/datetime/src/Makefile
panels/datetime/data/Makefile

panels/devices/Makefile
panels/devices/data/Makefile
panels/devices/data/dawati-panel-devices.schemas
panels/devices/src/Makefile
panels/devices/tests/Makefile

panels/music/Makefile

panels/myzone/Makefile
panels/myzone/capplet/Makefile
panels/myzone/data/Makefile
panels/myzone/data/dawati-panel-myzone.schemas
panels/myzone/src/Makefile

panels/networks/Makefile
panels/networks/src/Makefile
panels/networks/tests/Makefile
panels/networks/data/Makefile

panels/people/Makefile
panels/people/anerley/Makefile
panels/people/anerley/anerley/Makefile
panels/people/anerley/data/Makefile
panels/people/anerley/src/Makefile
panels/people/anerley/tests/Makefile
panels/people/data/Makefile
panels/people/src/Makefile

panels/power-icon/Makefile
panels/power-icon/data/Makefile
panels/power-icon/src/Makefile
panels/power-icon/tests/Makefile

panels/status/Makefile
panels/status/data/Makefile
panels/status/src/Makefile

panels/web/Makefile
panels/web/common/Makefile
panels/web/data/Makefile
panels/web/netpanel/Makefile

panels/switcher/Makefile
panels/switcher/data/Makefile
panels/switcher/src/Makefile

panels/bluetooth/Makefile
panels/bluetooth/src/Makefile
panels/bluetooth/data/Makefile
])

echo "
        Mutter Dawati Netbook Plugin version plugin_version
        ===================================================

        Configuration:

            Installation path:    ${MUTTER_PLUGIN_DIR}
	    Debug            :    ${enable_debug}
	    Profile          :    ${enable_profile}
	    Scaled background:    ${scale_background}
            API Reference    :    ${enable_gtk_doc}
            Panels           :    ${WITH_PANELS}
            Zeitgeist        :    ${enable_zeitgeist}
	    CFLAGS: ${CFLAGS}
"

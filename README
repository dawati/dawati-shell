
This is the Meego Netbook plugin for Metacity Clutter, aka, Mutter

Significant Preferences
=======================

The following Metacity preferences are of possible interest:

  clutter_disabled: if set to true, disables the clutter compositor
                    falling back to the xrender compositor.

  clutter_plugins:  list of plugins to load; name of each plugin can be
                    followed by an optional command line for the plugin
                    (separated by colon).

                    There are two globally defined command line parameters:

                      debug  the plugin should switch into debugging mode; the
                             precise significance of this is left to the plugin.

                      disable: followed by a semicolon-separate list of effects
                             that should not be utilized; possible values are
                             minimize, maximize, unmaximize, map, destroy, and
                             switch-workspace.


                    Example: 'default:debug disable:unmaximize'

		    When no plugin is specified, attempt is made to load
                    plugin called 'default'.


  live_hidden_windows:
                    If set to true, the window manager keeps windows that are
                    minimized or on non-active desktops mapped, so they can
                    be used by the compositor for things like window and
                    workspace switchers.

  dynamic_workspaces:
                   If set to true, the window manager will create a new
                   workspace for any application window that covers more than
                   75% of the screen. If no other windows are subsequently
                   placed on the same workspace later, it is automatically
                   removed when the window closes.

Dependencies:

MX: git.meego.org/mx.git
jana: svn.gnome.org/svn/jana
mojito: git.meego.org/mojito.git

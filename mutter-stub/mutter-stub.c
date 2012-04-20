#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter-gtk/clutter-gtk.h>
#include <clutter/x11/clutter-x11.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <meta/display.h>
#include <meta/compositor-mutter.h>
#include <meta/meta-plugin.h>
#include <meta/main.h>

#include <stdlib.h>

#define GRAB_MASK (StructureNotifyMask |                                \
                   ButtonPressMask | ButtonReleaseMask | PointerMotionMask | \
                   FocusChangeMask |                                    \
                   ExposureMask |                                       \
                   KeyPressMask | KeyReleaseMask |                      \
                   EnterWindowMask | LeaveWindowMask |                  \
                   PropertyChangeMask)

GOptionContext *
meta_get_option_context (void)
{
  GOptionContext *ctx;

  ctx = g_option_context_new (NULL);

  return ctx;
}

static int stub_exit_code = 0;
static GdkDisplay *stub_gdk_display;
static ClutterActor *stub_stage = NULL;
static ClutterActor *stub_group_windows = NULL;
static ClutterActor *stub_group_overlay = NULL;
static GType stub_plugin_type;
static MetaPlugin *stub_plugin = NULL;
static MetaScreen *stub_screen = NULL;
static MetaDisplay *stub_display = NULL;
static MetaWorkspace *stub_workspace = NULL;
static GList *stub_workspace_list = NULL;

static gboolean stub_in_grab = FALSE;

/**
 * Main stuff
 */

static GdkFilterReturn
gdk_to_clutter_event_pump__ (GdkXEvent *xevent,
                             GdkEvent  *event,
                             gpointer   data)
{
  GdkDisplay *display = gdk_display_get_default ();
  GdkWindow *gdk_win;

  XEvent *xev = (XEvent *) xevent;
  guint32 timestamp = 0;

  gdk_win = gdk_x11_window_lookup_for_display (display, xev->xany.window);

  /* if (xev->xany.type == KeyPress) */
  /*   { */
  /*     if (xev->xkey.state & (ControlMask | ShiftMask)) */
  /*       { */
  /*         GdkDeviceManager *manager = gdk_display_get_device_manager (display); */
  /*         GdkDevice *pointer_device = gdk_device_manager_get_client_pointer (manager); */

  /*         if (stub_in_grab) */
  /*           { */
  /*             gdk_device_ungrab (pointer_device, GDK_CURRENT_TIME); */
  /*           } */
  /*         else */
  /*           { */
  /*             gdk_device_grab (pointer_device, */
  /*                              NULL, */
  /*                              GDK_OWNERSHIP_NONE, */
  /*                              TRUE, */
  /*                              GRAB_MASK, */
  /*                              NULL, */
  /*                              GDK_CURRENT_TIME); */
  /*           } */

  /*         stub_in_grab = !stub_in_grab; */
  /*       } */
  /*   } */

  /*
   * Ensure we update the user time on this window if the event
   * implies user action.
   */
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
      case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      timestamp = event->button.time;
      break;
    case GDK_MOTION_NOTIFY:
      timestamp = event->motion.time;
      break;
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      timestamp = event->key.time;
      break;
    default: ;
    }

  if (timestamp && gdk_win)
    gdk_x11_window_set_user_time (gdk_win, timestamp);

  switch (clutter_x11_handle_event (xev))
    {
    default:
    case CLUTTER_X11_FILTER_CONTINUE:
      return GDK_FILTER_CONTINUE;
    case CLUTTER_X11_FILTER_TRANSLATE:
      return GDK_FILTER_TRANSLATE;
    case CLUTTER_X11_FILTER_REMOVE:
      return GDK_FILTER_REMOVE;
    }
};

void
meta_init (void)
{
  GdkWindow *window;

  if (clutter_init (NULL, NULL) != CLUTTER_INIT_SUCCESS)
    return;

  gtk_init (NULL, NULL);

  stub_gdk_display = gdk_display_get_default ();

  stub_stage = clutter_stage_new ();
  clutter_actor_set_size (stub_stage, 1024, 768);
  clutter_actor_realize (stub_stage);
  g_signal_connect (stub_stage, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  window =
    gdk_x11_window_foreign_new_for_display (gdk_display_get_default (),
                                            clutter_x11_get_stage_window (CLUTTER_STAGE (stub_stage)));
  XSelectInput (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                gdk_x11_window_get_xid (GDK_X11_WINDOW (window)),
                GRAB_MASK);

  gdk_window_add_filter (window, gdk_to_clutter_event_pump__, NULL);

  stub_group_windows = clutter_actor_new ();
  clutter_actor_add_child (stub_stage, stub_group_windows);
  stub_group_overlay = clutter_actor_new ();
  clutter_actor_add_child (stub_stage, stub_group_overlay);

  stub_plugin = g_object_new (stub_plugin_type, NULL);
  stub_screen = g_object_new (META_TYPE_SCREEN, NULL);
  stub_display = g_object_new (META_TYPE_DISPLAY, NULL);
  stub_workspace = g_object_new (META_TYPE_WORKSPACE, NULL);
  stub_workspace_list = g_list_append (NULL, stub_workspace);
}

int
meta_run (void)
{
  MetaPluginClass *klass = META_PLUGIN_GET_CLASS (stub_plugin);

  klass->start (stub_plugin);

  clutter_actor_show (stub_stage);
  gtk_main ();

  return stub_exit_code;
}

void
meta_quit (MetaExitCode code)
{
  stub_exit_code = code;
  gtk_main_quit ();
}

void
meta_exit (MetaExitCode code)
{
  exit (code);
}

gboolean
meta_get_replace_current_wm (void)
{
  return FALSE;
}

/**
 * Plugin stuff
 */

void
meta_plugin_type_register (GType plugin_type)
{
  stub_plugin_type = plugin_type;
}

/**
 * meta_plugin_get_screen:
 * @plugin: a #MetaPlugin
 *
 * Gets the #MetaScreen corresponding to a plugin. Each plugin instance
 * is associated with exactly one screen; if Metacity is managing
 * multiple screens, multiple plugin instances will be created.
 *
 * Return value: (transfer none): the #MetaScreen for the plugin
 */
MetaScreen *
meta_plugin_get_screen (MetaPlugin *plugin)
{
  return stub_screen;
}

void
meta_plugin_switch_workspace_completed (MetaPlugin *plugin)
{

}

void
meta_plugin_minimize_completed (MetaPlugin      *plugin,
                                MetaWindowActor *actor)
{

}

void
meta_plugin_maximize_completed (MetaPlugin      *plugin,
                                MetaWindowActor *actor)
{

}

void
meta_plugin_unmaximize_completed (MetaPlugin      *plugin,
                                  MetaWindowActor *actor)
{

}

void
meta_plugin_map_completed (MetaPlugin      *plugin,
                           MetaWindowActor *actor)
{

}

void
meta_plugin_destroy_completed (MetaPlugin      *plugin,
                               MetaWindowActor *actor)
{

}

gboolean
meta_plugin_begin_modal (MetaPlugin      *plugin,
                         Window           grab_window,
                         Cursor           cursor,
                         MetaModalOptions options,
                         guint32          timestamp)
{
  return TRUE;
}

void
meta_plugin_end_modal (MetaPlugin *plugin,
                       guint32     timestamp)
{

}


/**
 * Screen stuff
 */

/**
 * meta_screen_get_display:
 * Retrieve the display associated with screen.
 * @screen: A #MetaScreen
 *
 * Returns: (transfer none): Display
 */
MetaDisplay *
meta_screen_get_display (MetaScreen *screen)
{
  return stub_display;
}

int
meta_screen_get_active_workspace_index (MetaScreen *screen)
{
  return meta_workspace_index (stub_workspace);
}

MetaWorkspace *
meta_screen_get_active_workspace (MetaScreen *screen)
{
  return stub_workspace;
}

/**
 * meta_screen_get_size:
 * @screen: A #MetaScreen
 * @width: (out): The width of the screen
 * @height: (out): The height of the screen
 *
 * Retrieve the size of the screen.
 */
void
meta_screen_get_size (MetaScreen *screen,
                      int        *width,
                      int        *height)
{
  gfloat swidth, sheight;

  clutter_actor_get_size (stub_stage, &swidth, &sheight);

  if (width)
    *width = swidth;
  if (height)
    *height = sheight;
}

/**
 * meta_screen_get_startup_sequences: (skip)
 * @screen:
 *
 * Return value: (transfer none): Currently active #SnStartupSequence items
 */
GSList *
meta_screen_get_startup_sequences (MetaScreen *screen)
{
  return NULL; /* WTF is that */
}

/**
 * meta_screen_get_workspaces: (skip)
 * @screen: a #MetaScreen
 *
 * Returns: (transfer none) (element-type Meta.Workspace): The workspaces for @screen
 */
GList *
meta_screen_get_workspaces (MetaScreen *screen)
{
  return stub_workspace_list;
}

int
meta_screen_get_n_workspaces (MetaScreen *screen)
{
  return g_list_length (stub_workspace_list);
}

int
meta_screen_get_screen_number (MetaScreen *screen)
{
  return 0;
}

int
meta_screen_get_n_monitors (MetaScreen *screen)
{
  return 1;
}

int
meta_screen_get_primary_monitor (MetaScreen *screen)
{
  return 0;
}

/**
 * meta_screen_get_monitor_geometry:
 * @screen: a #MetaScreen
 * @monitor: the monitor number
 * @geometry: (out): location to store the monitor geometry
 *
 * Stores the location and size of the indicated monitor in @geometry.
 */
void
meta_screen_get_monitor_geometry (MetaScreen    *screen,
                                  int            monitor,
                                  MetaRectangle *geometry)
{
  if (geometry)
    {
      geometry->x = 0;
      geometry->y = 0;
      geometry->width = clutter_actor_get_width (stub_stage);
      geometry->height = clutter_actor_get_height (stub_stage);
    }
}


/**
 * Display stuff
 */

/**
 * meta_display_sort_windows_by_stacking:
 * @display: a #MetaDisplay
 * @windows: (element-type MetaWindow): Set of windows
 *
 * Sorts a set of windows according to their current stacking order. If windows
 * from multiple screens are present in the set of input windows, then all the
 * windows on screen 0 are sorted below all the windows on screen 1, and so forth.
 * Since the stacking order of override-redirect windows isn't controlled by
 * Metacity, if override-redirect windows are in the input, the result may not
 * correspond to the actual stacking order in the X server.
 *
 * An example of using this would be to sort the list of transient dialogs for a
 * window into their current stacking order.
 *
 * Returns: (transfer container) (element-type MetaWindow): Input windows sorted by stacking order, from lowest to highest
 */
GSList *
meta_display_sort_windows_by_stacking (MetaDisplay *display,
                                       GSList      *windows)
{
  return g_slist_copy (windows);
}

/**
 * meta_display_get_focus_window:
 * @display: a #MetaDisplay
 *
 * Get the window that, according to events received from X server,
 * currently has the input focus. We may have already sent a request
 * to the X server to move the focus window elsewhere. (The
 * expected_focus_window records where we've last set the input
 * focus.)
 *
 * Return Value: (transfer none): The current focus window
 */
MetaWindow *
meta_display_get_focus_window (MetaDisplay *display)
{
  return NULL;
}

guint32
meta_display_get_last_user_time (MetaDisplay *display)
{
  return 0;
}

guint32
meta_display_get_current_time (MetaDisplay *display)
{
  return 0;
}

gboolean
meta_display_xserver_time_is_before (MetaDisplay *display,
                                     guint32      time1,
                                     guint32      time2)
{
  return time1 < time2;
}

/**
 * meta_display_get_xdisplay: (skip)
 *
 */
Display *
meta_display_get_xdisplay (MetaDisplay *display)
{
  return gdk_x11_display_get_xdisplay (stub_gdk_display);
}

void meta_display_unmanage_screen (MetaDisplay *display,
                                   MetaScreen  *screen,
                                   guint32      timestamp)
{

}

void meta_display_focus_the_no_focus_window (MetaDisplay *display,
                                             MetaScreen  *screen,
                                             guint32      timestamp)
{

}


/**
 * Group stuff
 */

/**
 * meta_group_list_windows:
 * @group: A #MetaGroup
 *
 * Returns: (transfer container) (element-type Meta.Window): List of windows
 */
GSList *
meta_group_list_windows (MetaGroup *group)
{
  return NULL;
}

/**
 * meta_window_get_group: (skip)
 *
 */
MetaGroup *
meta_window_get_group (MetaWindow *window)
{
  return NULL;
}


/**
 * Window stuff
 */

const char *
meta_window_get_description (MetaWindow *window)
{
  return "Window description";
}

const char *
meta_window_get_wm_class (MetaWindow *window)
{
  return "Window WM class";
}

const char *
meta_window_get_wm_class_instance (MetaWindow *window)
{
  return "Window WM class instance";
}

void
meta_window_foreach_transient (MetaWindow            *window,
                               MetaWindowForeachFunc  func,
                               void                  *user_data)
{

}

MetaWindowType
meta_window_get_window_type (MetaWindow *window)
{
  return META_WINDOW_NORMAL;
}

void
meta_window_maximize (MetaWindow        *window,
                      MetaMaximizeFlags  directions)
{

}

void
meta_window_unmaximize (MetaWindow        *window,
                        MetaMaximizeFlags  directions)
{

}

void
meta_window_minimize (MetaWindow  *window)
{

}

void
meta_window_unminimize (MetaWindow  *window)
{

}

void
meta_window_raise (MetaWindow  *window)
{

}

void
meta_window_lower (MetaWindow  *window)
{

}

const char *
meta_window_get_title (MetaWindow *window)
{
  return "Window title";
}

/**
 * meta_window_get_workspace:
 * @window: a #MetaWindow
 *
 * Gets the #MetaWorkspace that the window is currently displayed on.
 * If the window is on all workspaces, returns the currently active
 * workspace.
 *
 * Return value: (transfer none): the #MetaWorkspace for the window
 */
MetaWorkspace *
meta_window_get_workspace (MetaWindow *window)
{
  return stub_workspace;
}

gboolean
meta_window_is_remote (MetaWindow *window)
{
  return FALSE;
}

int
meta_window_get_pid (MetaWindow *window)
{
  return 42;
}

/**
 * meta_window_get_transient_for:
 * @window: a #MetaWindow
 *
 * Returns the #MetaWindow for the window that is pointed to by the
 * WM_TRANSIENT_FOR hint on this window (see XGetTransientForHint()
 * or XSetTransientForHint()). Metacity keeps transient windows above their
 * parents. A typical usage of this hint is for a dialog that wants to stay
 * above its associated window.
 *
 * Return value: (transfer none): the window this window is transient for, or
 * %NULL if the WM_TRANSIENT_FOR hint is unset or does not point to a toplevel
 * window that Metacity knows about.
 */
MetaWindow *
meta_window_get_transient_for (MetaWindow *window)
{
  return NULL;
}

const char *
meta_window_get_startup_id (MetaWindow *window)
{
  return "Window startup id";
}

void
meta_window_activate (MetaWindow *window, guint32 current_time)
{

}

guint32
meta_window_get_user_time (MetaWindow *window)
{
  return 0; /* TODO */
}

void
meta_window_delete (MetaWindow  *window,
                    guint32      timestamp)
{

}

gboolean
meta_window_showing_on_its_workspace (MetaWindow *window)
{
  return TRUE;
}

gboolean
meta_window_is_shaded (MetaWindow *window)
{
  return FALSE;
}

gboolean
meta_window_is_override_redirect (MetaWindow *window)
{
  return FALSE;
}

gboolean
meta_window_is_skip_taskbar (MetaWindow *window)
{
  return FALSE;
}

guint
meta_window_get_stable_sequence (MetaWindow *window)
{
  return 0;
}

void
meta_window_set_demands_attention (MetaWindow *window)
{

}

void
meta_window_unset_demands_attention (MetaWindow *window)
{

}


/**
 * Workspace stuff
 */

int
meta_workspace_index (MetaWorkspace *workspace)
{
  return 0;
}

/**
 * meta_workspace_list_windows:
 * @workspace: a #MetaWorkspace
 *
 * Gets windows contained on the workspace, including workspace->windows
 * and also sticky windows. Override-redirect windows are not included.
 *
 * Return value: (transfer container) (element-type MetaWindow): the list of windows.
 */
GList *
meta_workspace_list_windows (MetaWorkspace *workspace)
{
  return NULL;
}

void
meta_workspace_activate (MetaWorkspace *workspace, guint32 timestamp)
{
}

void
meta_workspace_activate_with_focus (MetaWorkspace *workspace,
                                    MetaWindow    *focus_this,
                                    guint32        timestamp)
{

}

const char *
meta_window_get_gtk_application_id (MetaWindow *window)
{
  return "Window GTK+ application id";
}

const char *
meta_window_get_gtk_unique_bus_name (MetaWindow *window)
{
  return "Window GTK+ bus name";
}

const char *
meta_window_get_gtk_application_object_path (MetaWindow *window)
{
  return "Window GTK+ application object path";
}

const char *
meta_window_get_gtk_window_object_path (MetaWindow *window)
{
  return "Window GTK+ window object path";
}

const char *
meta_window_get_gtk_app_menu_object_path (MetaWindow *window)
{
  return "Window GTK+ app menu object path";
}

const char *
meta_window_get_gtk_menubar_object_path (MetaWindow *window)
{
  return "Window GTK+ menubar object path";
}


/**
 * Compositor stuff
 */

/**
 * meta_get_stage_for_screen:
 * @screen: a #MetaScreen
 *
 * Returns: (transfer none): The #ClutterStage for the screen
 */
ClutterActor *
meta_get_stage_for_screen (MetaScreen *screen)
{
  return stub_stage;
}

/**
 * meta_get_overlay_group_for_screen:
 * @screen: a #MetaScreen
 *
 * Returns: (transfer none): The overlay group corresponding to @screen
 */
ClutterActor *
meta_get_overlay_group_for_screen (MetaScreen *screen)
{
  return stub_group_overlay;
}

Window
meta_get_overlay_window (MetaScreen *screen)
{
  return 0; /* TODO: No idea what to do though... */
}

/**
 * meta_get_window_actors:
 * @screen: a #MetaScreen
 *
 * Returns: (transfer none) (element-type Clutter.Actor): The set of #MetaWindowActor on @screen
 */
GList *
meta_get_window_actors (MetaScreen *screen)
{
  return NULL; /* No window for now */
}

/**
 * meta_get_window_group_for_screen:
 * @screen: a #MetaScreen
 *
 * Returns: (transfer none): The window group corresponding to @screen
 */
ClutterActor *
meta_get_window_group_for_screen (MetaScreen *screen)
{
  return stub_group_windows;
}

void
meta_disable_unredirect_for_screen (MetaScreen *screen)
{

}

void
meta_enable_unredirect_for_screen (MetaScreen *screen)
{

}

void
meta_set_stage_input_region (MetaScreen    *screen,
                             XserverRegion  region)
{

}

void
meta_empty_stage_input_region (MetaScreen    *screen)
{

}

/**
 * meta_get_background_actor_for_screen:
 * @screen: a #MetaScreen
 *
 * Gets the actor that draws the root window background under the windows.
 * The root window background automatically tracks the image or color set
 * by the environment.
 *
 * Returns: (transfer none): The background actor corresponding to @screen
 */
ClutterActor *
meta_get_background_actor_for_screen (MetaScreen *screen)
{
  return NULL; /**/
}

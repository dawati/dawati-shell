/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Based on Gnome Shell.
 */
#include <clutter/clutter.h>
#include <clutter/glx/clutter-glx.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtk.h>
#include <nbtk/nbtk.h>
#include <string.h>

#include "mnb-panel-button.h"
#include "moblin-netbook-tray-manager.h"
#include "tray/na-tray-manager.h"
#include "moblin-netbook.h"
#include "moblin-netbook-panel.h"

#define MOBLIN_SYSTEM_TRAY_FROM_PLUGIN
#include "moblin-netbook-system-tray.h"

#define TRAY_BUTTON_HEIGHT 55
#define TRAY_BUTTON_WIDTH 44

typedef enum
{
  CHILD_UNKNOWN = 0,
  CHILD_BLUETOOTH,
  CHILD_WIFI,
  CHILD_SOUND,
  CHILD_BATTERY,
  CHILD_TEST,
} ChildType;

struct _ShellTrayManagerPrivate {
  NaTrayManager *na_manager;
  ClutterStage *stage;
  GdkWindow *stage_window;
  ClutterColor bg_color;

  GHashTable *icons;

  GList *config_windows;

  MutterPlugin *plugin;
};

typedef struct {
  ShellTrayManager *manager;
  GtkWidget *socket;
  GtkWidget *window;
  GtkWidget *config;
  Window     config_xwin;
  ClutterActor *actor;
  MnbInputRegion mir;
  guint timeout_count;
  guint timeout_id;
} ShellTrayManagerChild;

enum {
  PROP_0,

  PROP_BG_COLOR,
  PROP_MUTTER_PLUGIN,
};

/* Signals */
enum
{
  TRAY_ICON_ADDED,
  TRAY_ICON_REMOVED,
  LAST_SIGNAL
};

G_DEFINE_TYPE (ShellTrayManager, shell_tray_manager, G_TYPE_OBJECT);

static guint shell_tray_manager_signals [LAST_SIGNAL] = { 0 };

/* Sea Green - obnoxious to force people to set the background color */
static const ClutterColor default_color = { 0xbb, 0xff, 0xaa };

static void na_tray_icon_added (NaTrayManager *na_manager, GtkWidget *child, gpointer manager);
static void na_tray_icon_removed (NaTrayManager *na_manager, GtkWidget *child, gpointer manager);

static void destroy_config_window (ShellTrayManagerChild *child);

static void
free_tray_icon (gpointer data)
{
  ShellTrayManagerChild *child   = data;
  ShellTrayManager      *manager = child->manager;
  MutterPlugin          *plugin  = manager->priv->plugin;

  destroy_config_window (child);

  if (child->timeout_id > 0)
    g_source_remove (child->timeout_id);

  gtk_widget_hide (child->window);
  gtk_widget_destroy (child->window);
  g_signal_handlers_disconnect_matched (child->actor, G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, child);

  if (child->mir)
    moblin_netbook_input_region_remove (plugin, child->mir);

  g_object_unref (child->actor);
  g_slice_free (ShellTrayManagerChild, child);
}

static void
shell_tray_manager_set_property(GObject         *object,
                                guint            prop_id,
                                const GValue    *value,
                                GParamSpec      *pspec)
{
  ShellTrayManager *manager = SHELL_TRAY_MANAGER (object);

  switch (prop_id)
    {
    case PROP_BG_COLOR:
      {
        ClutterColor *color = g_value_get_boxed (value);
        if (color)
          manager->priv->bg_color = *color;
        else
          manager->priv->bg_color = default_color;
      }
      break;
    case PROP_MUTTER_PLUGIN:
      {
        manager->priv->plugin = g_value_get_object (value);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
shell_tray_manager_get_property(GObject         *object,
                                guint            prop_id,
                                GValue          *value,
                                GParamSpec      *pspec)
{
  ShellTrayManager *manager = SHELL_TRAY_MANAGER (object);

  switch (prop_id)
    {
    case PROP_BG_COLOR:
      g_value_set_boxed (value, &manager->priv->bg_color);
      break;
    case PROP_MUTTER_PLUGIN:
      g_value_set_object (value, manager->priv->plugin);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
shell_tray_manager_init (ShellTrayManager *manager)
{
  manager->priv = G_TYPE_INSTANCE_GET_PRIVATE (manager, SHELL_TYPE_TRAY_MANAGER,
                                               ShellTrayManagerPrivate);
  manager->priv->na_manager = na_tray_manager_new ();

  manager->priv->icons = g_hash_table_new_full (NULL, NULL,
                                                NULL, free_tray_icon);
  manager->priv->bg_color = default_color;

  g_signal_connect (manager->priv->na_manager, "tray-icon-added",
                    G_CALLBACK (na_tray_icon_added), manager);
  g_signal_connect (manager->priv->na_manager, "tray-icon-removed",
                    G_CALLBACK (na_tray_icon_removed), manager);
}

static void
shell_tray_manager_finalize (GObject *object)
{
  ShellTrayManager *manager = SHELL_TRAY_MANAGER (object);

  g_object_unref (manager->priv->na_manager);
  g_object_unref (manager->priv->stage);
  g_object_unref (manager->priv->stage_window);
  g_hash_table_destroy (manager->priv->icons);

  G_OBJECT_CLASS (shell_tray_manager_parent_class)->finalize (object);
}

static void
shell_tray_manager_class_init (ShellTrayManagerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ShellTrayManagerPrivate));

  gobject_class->finalize = shell_tray_manager_finalize;
  gobject_class->set_property = shell_tray_manager_set_property;
  gobject_class->get_property = shell_tray_manager_get_property;

  shell_tray_manager_signals[TRAY_ICON_ADDED] =
    g_signal_new ("tray-icon-added",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ShellTrayManagerClass, tray_icon_added),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
                  CLUTTER_TYPE_ACTOR);
  shell_tray_manager_signals[TRAY_ICON_REMOVED] =
    g_signal_new ("tray-icon-removed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ShellTrayManagerClass, tray_icon_removed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
                  CLUTTER_TYPE_ACTOR);

  /* Lifting the CONSTRUCT_ONLY here isn't hard; you just need to
   * iterate through the icons, reset the background pixmap, and
   * call na_tray_child_force_redraw()
   */
  g_object_class_install_property (gobject_class,
                                   PROP_BG_COLOR,
                                   g_param_spec_boxed ("bg-color",
                                                       "BG Color",
                                                       "Background color (only if we don't have transparency)",
                                                       CLUTTER_TYPE_COLOR,
                                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class,
                                   PROP_MUTTER_PLUGIN,
                                   g_param_spec_object ("mutter-plugin",
                                                        "Mutter Plugin",
                                                        "Mutter Plugin",
                                                        MUTTER_TYPE_PLUGIN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

ShellTrayManager *
shell_tray_manager_new (void)
{
  return g_object_new (SHELL_TYPE_TRAY_MANAGER, NULL);
}

void
shell_tray_manager_manage_stage (ShellTrayManager *manager,
                                 ClutterStage     *stage)
{
  Window stage_xwin;

  g_return_if_fail (manager->priv->stage == NULL);

  manager->priv->stage = g_object_ref (stage);
  stage_xwin = clutter_x11_get_stage_window (stage);
  manager->priv->stage_window = gdk_window_lookup (stage_xwin);
  if (manager->priv->stage_window)
    g_object_ref (manager->priv->stage_window);
  else
    manager->priv->stage_window = gdk_window_foreign_new (stage_xwin);

  na_tray_manager_manage_screen (manager->priv->na_manager,
                                 gdk_drawable_get_screen (GDK_DRAWABLE (manager->priv->stage_window)));
}

static GdkPixmap *
create_bg_pixmap (GdkColormap  *colormap,
                  ClutterColor *color)
{
  GdkScreen *screen = gdk_colormap_get_screen (colormap);
  GdkVisual *visual = gdk_colormap_get_visual (colormap);
  GdkPixmap *pixmap = gdk_pixmap_new (gdk_screen_get_root_window (screen),
                                      1, 1,
                                      visual->depth);

  cairo_t *cr = gdk_cairo_create (pixmap);
  cairo_set_source_rgb (cr,
                        color->red / 255.,
                        color->green / 255.,
                        color->blue / 255.);
  cairo_paint (cr);
  cairo_destroy (cr);

  return pixmap;
}

static void
destroy_config_window (ShellTrayManagerChild *child)
{
  if (child->config)
    {
      GtkWidget                  *config  = child->config;
      ShellTrayManager           *manager = child->manager;
      MutterPlugin               *plugin  = manager->priv->plugin;

      if (child->mir)
        {
          moblin_netbook_input_region_remove (plugin, child->mir);
          child->mir = NULL;
        }

      manager->priv->config_windows =
        g_list_remove (manager->priv->config_windows,
                       GINT_TO_POINTER (child->config_xwin));

      child->config = NULL;
      gtk_widget_destroy (config);

      nbtk_button_set_checked (NBTK_BUTTON (child->actor), FALSE);
    }
}

static gboolean
config_plug_removed_cb (GtkSocket *socket, gpointer data)
{
  ShellTrayManagerChild *child = data;

  destroy_config_window (child);

  return FALSE;
}

/*
 * GtkSocket does not provide any mechanism for tracking when the plug window
 * (un)maps (it would be easy to do by just adding couple of signals since
 * it the socket actually tracks the map state). But at least the socket gets
 * resized when the plug unmaps, so we track the size changes and watch it's
 * is_mapped member.
 */
static void
config_socket_size_allocate_cb (GtkWidget     *widget,
                                GtkAllocation *allocation,
                                gpointer       data)
{
  GtkSocket *socket = GTK_SOCKET (widget);

  if (!socket->is_mapped)
    {
      ShellTrayManagerChild *child = data;

      destroy_config_window (child);
      hide_panel (child->manager->priv->plugin);
    }
  else
    {
      ShellTrayManagerChild *child = data;
      ShellTrayManager *manager = child->manager;
      MutterPlugin *plugin = manager->priv->plugin;
      gint x = 0, y = 0, w, h, sw, sh;

      if (child->actor)
        clutter_actor_get_transformed_position (child->actor, &x, &y);

      w = child->config->allocation.width;
      h = child->config->allocation.height;

      mutter_plugin_query_screen_size (plugin, &sw, &sh);

      y = PANEL_HEIGHT;

      if (x + w > (sw - 10)) /* FIXME -- query panel padding */
        {
          /*
           * The window would be stretching past the screen edge, move
           * it left.
           */
          x = (sw - 10) - w;
        }

      gtk_window_move (GTK_WINDOW (child->config), x, y);

      /*
       * Cut out a hole into the stage input mask matching the config
       * window.
       */
      if (child->mir)
        moblin_netbook_input_region_remove_without_update (plugin, child->mir);

      child->mir = moblin_netbook_input_region_push (plugin, x, y,
                                                     (guint)w, (guint)h, TRUE);
    }
}

/*
 * Retrieves the type of the application.
 *
 * Return: 0 if the application is not one of the supported ones, 1 if it is
 * supported and < 0 on error. (NB: return value of 1 does not mean there is a
 * config window).
 */
static gint
check_child (ShellTrayManagerChild *child)
{
  if (!child->config)
    {
      static Atom tray_type = None;

      ShellTrayManager      *manager = child->manager;
      MutterPlugin          *plugin  = manager->priv->plugin;
      Display               *xdpy    = mutter_plugin_get_xdisplay (plugin);
      GtkSocket             *socket  = GTK_SOCKET (child->socket);
      Window                 xwin = GDK_WINDOW_XWINDOW (socket->plug_window);
      gulong                 n_items, left;
      gint                   ret_fmt;
      Atom                   ret_type;
      ChildType              child_type = 0;
      char                  *my_type = NULL;
      gint                   error_code;

      if (!tray_type)
        tray_type = XInternAtom (xdpy, MOBLIN_SYSTEM_TRAY_TYPE, False);


      gdk_error_trap_push ();
      XGetWindowProperty (xdpy, xwin, tray_type, 0, 8192, False,
                          XA_STRING, &ret_type, &ret_fmt, &n_items, &left,
                          (unsigned char**)&my_type);
      gdk_flush ();

      if ((error_code = gdk_error_trap_pop ()))
        {
          g_debug ("Got X Error %d when trying to query properties on 0x%x\n",
                   error_code, (guint)xwin);

          return -1;
        }

      if (my_type && *my_type)
        {
          if (!strcmp (my_type, "bluetooth"))
            {
              child_type = CHILD_BLUETOOTH;
              clutter_actor_set_name (child->actor, "tray-button-bluetooth");
            }
          else if (!strcmp (my_type, "wifi"))
            {
              child_type = CHILD_WIFI;
              clutter_actor_set_name (child->actor, "tray-button-wifi");
            }
          else if (!strcmp (my_type, "sound"))
            {
              child_type = CHILD_SOUND;
              clutter_actor_set_name (child->actor, "tray-button-sound");
            }
          else if (!strcmp (my_type, "battery"))
            {
              child_type = CHILD_BATTERY;
              clutter_actor_set_name (child->actor, "tray-button-battery");
            }
          else if (!strcmp (my_type, "test"))
            {
              child_type = CHILD_TEST;
              clutter_actor_set_name (child->actor, "tray-button-test");
            }

          XFree (my_type);
        }

      nbtk_button_set_checked (NBTK_BUTTON (child->actor), FALSE);

      if (child_type != CHILD_UNKNOWN)
        return 1;
    }

  return 0;
}

/*
 * Retrieves, if it exists, the ID of the child config window and embeds it in a
 * socket.
 *
 * The caller must check that child->config actually exist after calling this
 * function.
 *
 * Return: TRUE if config window was found.
 */
static gboolean
setup_child_config (ShellTrayManagerChild *child)
{
  if (!child->config)
    {
      static Atom tray_atom = None;

      ShellTrayManager      *manager = child->manager;
      MutterPlugin          *plugin  = manager->priv->plugin;
      Display               *xdpy    = mutter_plugin_get_xdisplay (plugin);
      GtkSocket             *socket  = GTK_SOCKET (child->socket);
      Window                 xwin = GDK_WINDOW_XWINDOW (socket->plug_window);
      Window                *config_xwin;
      GtkWidget             *config;
      GtkWidget             *config_socket;
      gulong                 n_items, left;
      gint                   ret_fmt;
      Atom                   ret_type;

      if (!tray_atom)
        tray_atom = XInternAtom (xdpy, MOBLIN_SYSTEM_TRAY_CONFIG_WINDOW, False);

      XGetWindowProperty (xdpy, xwin, tray_atom, 0, 8192, False,
                          XA_WINDOW, &ret_type, &ret_fmt, &n_items, &left,
                          (unsigned char **)&config_xwin);

      if (!config_xwin)
        {
          /*
           * This is a supported application, but it does not provide a
           * config window, return TRUE.
           */
          nbtk_button_set_checked (NBTK_BUTTON (child->actor), FALSE);
          return TRUE;
        }

      config_socket = gtk_socket_new ();
      child->config = config = gtk_window_new (GTK_WINDOW_POPUP);

      gtk_container_add (GTK_CONTAINER (config), config_socket);
      gtk_socket_add_id (GTK_SOCKET (config_socket), *config_xwin);

      if (!GTK_SOCKET (config_socket)->is_mapped)
        {
          child->config = NULL;
          child->config_xwin = None;

          gtk_widget_destroy (config);
          return FALSE;
        }
      else
        {
          gtk_widget_realize (config);

          child->config_xwin = GDK_WINDOW_XID (config->window);

          g_signal_connect (config_socket, "size-allocate",
                            G_CALLBACK (config_socket_size_allocate_cb),
                            child);

          g_signal_connect (config_socket, "plug-removed",
                            G_CALLBACK (config_plug_removed_cb), child);
        }

      XFree (config_xwin);
    }

  return TRUE;
}

/*
 * Callback for the "clicked" signal from NbtkButton.
 */
static gboolean
actor_clicked (ClutterActor *actor, gpointer data)
{
  ShellTrayManagerChild *child = data;
  gboolean               active;

  active = nbtk_button_get_checked (NBTK_BUTTON (actor));

  if (active)
    {
      /*
       * If we have a config window already constructed, show it, otherwise
       * create it first.
       */
      if (child->config || (setup_child_config (child) && child->config))
        {
          ShellTrayManager *manager = child->manager;

          manager->priv->config_windows =
            g_list_prepend (manager->priv->config_windows,
                            GINT_TO_POINTER (
                                       GDK_WINDOW_XID (child->config->window)));

          gtk_widget_show_all (child->config);
        }
    }
  else if (child->config)
    {
      destroy_config_window (child);
    }

  return TRUE;
}

/*
 * The application can only attach the config window id to the icon
 * once the icons has been embedded; because of the assynchronous nature of
 * this all, we cannot check for this in the in the tray-icon-added signal
 * handler. So we use a short timeout, which we allow to run up to 6 times.
 * If the application does not tag the icon with the config window, we
 * shun it.
 */
static gboolean
tray_icon_tagged_timeout_cb (gpointer data)
{
  ShellTrayManagerChild *child = data;
  gint                   config_status;

  if (child->config)
  {
    child->timeout_id = 0;
    return FALSE;
  }

  if ((config_status = check_child (child)) == 0)
    {
      if (child->timeout_count++ < 5)
        {
          /*
           * Give the application a bit longer ...
           */
          return TRUE;
        }

        g_warning ("No config window attached, expelling tray application\n.");

        /*
         * We leave the child in the hashtable; this keep the status icon
         * formally embedded and prevents the application from trying to get
         * embedded again and again and again.
         */
    }
  else if (config_status > 0)
    {
      ShellTrayManager *manager = child->manager;

      /*
       * Ok, only now that we know this application is OK we emit the
       * tray-icon-added signal (this results in the button being inserted
       * into the panel), and connect to the button's "clicked" signal.
       */
      g_signal_emit (manager,
                     shell_tray_manager_signals[TRAY_ICON_ADDED], 0,
                     child->actor);

      g_signal_connect (child->actor, "clicked",
                        G_CALLBACK (actor_clicked), child);
    }

  child->timeout_id = 0;
  return FALSE;
}

/*
 * ARGB icons have to be painted manually on expose.
 */
static gboolean
na_tray_expose_child (GtkWidget             *window,
                      GdkEventExpose        *event,
                      ShellTrayManagerChild *child)
{
  cairo_t *cr;

  if (!na_tray_child_is_composited (NA_TRAY_CHILD (child->socket)))
    return FALSE;

  cr = gdk_cairo_create (child->window->window);

  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  gdk_cairo_set_source_pixmap (cr,
                               child->socket->window,
                               child->socket->allocation.x,
                               child->socket->allocation.y);

  cairo_paint (cr);
  cairo_destroy (cr);

  return FALSE;
}

static void
na_tray_icon_added (NaTrayManager *na_manager, GtkWidget *socket,
                    gpointer user_data)
{
  static gint offset = 0;

  ShellTrayManager *manager = user_data;
  GtkWidget *win;
  ClutterActor *icon;
  ShellTrayManagerChild *child;
  GdkPixmap *bg_pixmap;
  NbtkWidget *button;
  child = g_slice_new0 (ShellTrayManagerChild);

  win = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_container_add (GTK_CONTAINER (win), socket);

  /* The colormap of the socket matches that of its contents; make
   * the window we put it in match that as well */
  gtk_widget_set_colormap (win, gtk_widget_get_colormap (socket));

  gtk_widget_set_size_request (win, 24, 24);
  gtk_widget_realize (win);
  gtk_widget_realize (socket);

  child->window  = win;
  child->socket  = socket;
  child->manager = manager;

  /* If the tray child is using an RGBA colormap (and so we have real
   * transparency), we don't need to worry about the background. If
   * not, we obey the bg-color property by creating a 1x1 pixmap of
   * that color and setting it as our background. Then "parent-relative"
   * background on the socket and the plug within that will cause
   * the icons contents to appear on top of our background color.
   *
   * Ordering warning: na_tray_child_is_composited() doesn't work
   *   until the tray child has been realized.
   */
  if (!na_tray_child_is_composited (NA_TRAY_CHILD (socket)))
    {
      bg_pixmap = create_bg_pixmap (gtk_widget_get_colormap (win),
                                    &manager->priv->bg_color);
      gdk_window_set_back_pixmap (win->window, bg_pixmap, FALSE);
      g_object_unref (bg_pixmap);
    }
  else
    {
      gtk_widget_set_app_paintable (win, TRUE);
      g_signal_connect (win, "expose-event",
                        G_CALLBACK (na_tray_expose_child), child);
    }

  gtk_widget_set_parent_window (win, manager->priv->stage_window);
  gdk_window_reparent (win->window, manager->priv->stage_window, 0, 0);
  gtk_window_move (GTK_WINDOW (win), offset, -200);
  offset += 32;
  gtk_widget_show_all (win);

  icon = clutter_glx_texture_pixmap_new_with_window (
                                             GDK_WINDOW_XWINDOW (win->window));
  clutter_x11_texture_pixmap_set_automatic (CLUTTER_X11_TEXTURE_PIXMAP (icon),
                                            TRUE);

  button = mnb_panel_button_new ();
  nbtk_button_set_toggle_mode (NBTK_BUTTON (button), TRUE);

  clutter_actor_set_size (CLUTTER_ACTOR (button),
                          TRAY_BUTTON_WIDTH, TRAY_BUTTON_HEIGHT);
  clutter_actor_set_name (CLUTTER_ACTOR (button), "tray-button");
  mnb_panel_button_set_reactive_area (MNB_PANEL_BUTTON (button),
                                      0,
                                      -(PANEL_HEIGHT - TRAY_BUTTON_HEIGHT),
                                      TRAY_BUTTON_WIDTH,
                                      PANEL_HEIGHT);

  clutter_container_add_actor (CLUTTER_CONTAINER (button), icon);

  clutter_actor_set_size (icon, 24, 24);
  clutter_actor_set_reactive (icon, TRUE);

  child->actor = g_object_ref (button);

  g_hash_table_insert (manager->priv->icons, socket, child);

  child->timeout_id = g_timeout_add (100, tray_icon_tagged_timeout_cb, child);
}

static void
na_tray_icon_removed (NaTrayManager *na_manager, GtkWidget *socket,
                      gpointer user_data)
{
  ShellTrayManager *manager = user_data;
  ShellTrayManagerChild *child;

  child = g_hash_table_lookup (manager->priv->icons, socket);

  if (!child)
    return;

  if (child->config)
    g_signal_emit (manager,
                   shell_tray_manager_signals[TRAY_ICON_REMOVED], 0,
                   child->actor);

  g_hash_table_remove (manager->priv->icons, socket);
}

gboolean
shell_tray_manager_is_config_window (ShellTrayManager *manager, Window xwindow)
{
  GList *l = manager->priv->config_windows;

  while (l)
    {
      Window xwin = GPOINTER_TO_INT (l->data);

      if (xwin == xwindow)
        return TRUE;

      l = l->next;
    }

  return FALSE;
}

gboolean
shell_tray_manager_config_windows_showing (ShellTrayManager *manager)
{
  GList *l = manager->priv->config_windows;

  if (!l)
    return FALSE;

  return TRUE;
}

static gboolean
find_child_data (gpointer key, gpointer value, gpointer data)
{
  ShellTrayManagerChild *child   = value;
  Window                 xwindow = GPOINTER_TO_INT (data);

  if (child->config_xwin == xwindow)
    return TRUE;

  return FALSE;
}

void
shell_tray_manager_hide_config_window (ShellTrayManager *manager,
                                       Window            xwindow)
{
  GHashTable            *icons = manager->priv->icons;
  ShellTrayManagerChild *child;

  child = g_hash_table_find (icons, find_child_data, GINT_TO_POINTER (xwindow));

  if (child && child->config)
    gtk_widget_hide (child->config);
}

void
shell_tray_manager_close_config_window (ShellTrayManager *manager,
                                        Window            xwindow)
{
  GHashTable            *icons = manager->priv->icons;
  ShellTrayManagerChild *child;

  child = g_hash_table_find (icons, find_child_data, GINT_TO_POINTER (xwindow));

  if (child)
    destroy_config_window (child);
}

void
shell_tray_manager_close_all_config_windows (ShellTrayManager *manager)
{
  GList *l = manager->priv->config_windows;

  while (l)
    {
      Window xwin = GPOINTER_TO_INT (l->data);

      shell_tray_manager_close_config_window (manager, xwin);

      l = manager->priv->config_windows;
    }
}

void
shell_tray_manager_close_all_other_config_windows (ShellTrayManager *manager,
                                                   Window            xwindow)
{
  GList *l = manager->priv->config_windows;

  while (l)
    {
      Window xwin = GPOINTER_TO_INT (l->data);

      if (xwindow != xwin)
        {
          shell_tray_manager_close_config_window (manager, xwin);
          l = manager->priv->config_windows;
        }
      else
        l = l->next;
    }
}


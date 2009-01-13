/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Based on Gnome Shell.
 */
#include <clutter/clutter.h>
#include <clutter/glx/clutter-glx.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtk.h>

#include "moblin-netbook-tray-manager.h"
#include "tray/na-tray-manager.h"
#include "moblin-netbook.h"
#include "moblin-netbook-panel.h"
#include "moblin-netbook-ui.h"

#define MOBLIN_SYSTEM_TRAY_FROM_PLUGIN
#include "moblin-netbook-system-tray.h"

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
  ShellTrayManagerChild *child = data;

  destroy_config_window (child);

  gtk_widget_hide (child->window);
  gtk_widget_destroy (child->window);
  g_signal_handlers_disconnect_matched (child->actor, G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, child);
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
      MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN(plugin)->priv;

      if (CLUTTER_ACTOR_IS_VISIBLE (priv->panel))
        enable_stage (plugin, CurrentTime);
      else
        disable_stage (plugin, CurrentTime);

      manager->priv->config_windows =
        g_list_remove (manager->priv->config_windows,
                       GINT_TO_POINTER (child->config_xwin));

      child->config = NULL;
      gtk_widget_destroy (config);
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
    }
}

static void
combine_config_windows_input_shape (ShellTrayManager *manager,
                                    Display          *xdpy,
                                    XserverRegion     dest)
{
  GList *l = manager->priv->config_windows;

  while (l)
    {
      Window xwin = GPOINTER_TO_INT (l->data);
      XserverRegion win_region;

      win_region = XFixesCreateRegionFromWindow (xdpy, xwin, 0);

      XFixesSubtractRegion (xdpy, dest, dest, win_region);
      XFixesDestroyRegion  (xdpy, win_region);

      l = l->next;
    }
}

static gboolean
actor_clicked (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  static Atom            tray_atom = 0;

  ShellTrayManagerChild *child   = data;
  ShellTrayManager      *manager = child->manager;
  MutterPlugin          *plugin  = manager->priv->plugin;
  MetaScreen            *screen  = mutter_plugin_get_screen (plugin);
  Display               *xdpy    = mutter_plugin_get_xdisplay (plugin);
  GtkSocket             *socket  = GTK_SOCKET (child->socket);
  ClutterActor          *stage   = mutter_plugin_get_stage (plugin);
  Window                 stage_win;

  stage_win = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

  if (!child->config)
    {
      Window     xwin = GDK_WINDOW_XWINDOW (socket->plug_window);
      Window    *config_xwin;
      GtkWidget *config;
      GtkWidget *config_socket;
      gulong     n_items, left;
      gint       ret_fmt;
      Atom       ret_type;

      if (!tray_atom)
        tray_atom = XInternAtom (xdpy, MOBLIN_SYSTEM_TRAY_CONFIG_WINDOW, False);

      XGetWindowProperty (xdpy, xwin, tray_atom, 0, 8192, False,
                          XA_WINDOW, &ret_type, &ret_fmt, &n_items, &left,
                          (unsigned char **)&config_xwin);

      if (!config_xwin)
        return;

      config_socket = gtk_socket_new ();
      child->config = config = gtk_window_new (GTK_WINDOW_POPUP);
      child->config_xwin = *config_xwin;

      gtk_container_add (GTK_CONTAINER (config), config_socket);
      gtk_socket_add_id (GTK_SOCKET (config_socket), *config_xwin);

      if (!GTK_SOCKET (config_socket)->is_mapped)
        {
          child->config = NULL;
          child->config_xwin = None;

          gtk_widget_destroy (config);
        }
      else
        {
          MoblinNetbookPluginPrivate *priv =
            MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
          XserverRegion window_region, comb_region;
          XRectangle    rect;
          gint          x = 0, y = 0, w, h, sw, sh;
          GList        *wins = manager->priv->config_windows;

          if (child->actor)
            clutter_actor_get_transformed_position (child->actor, &x, &y);

          gtk_widget_realize (config);
          gtk_window_get_size (GTK_WINDOW (config), &w, &h);

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

          gtk_window_move (GTK_WINDOW (config), x, y);

          /*
           * Cut out a hole into the stage input mask matching the config
           * window.
           */
          rect.x      = x;
          rect.y      = y;
          rect.width  = w;
          rect.height = h;

          window_region = XFixesCreateRegion (xdpy, &rect, 1);
          comb_region = XFixesCreateRegion (xdpy, NULL, 0);

          XFixesSubtractRegion (xdpy, comb_region,
                                priv->screen_region,
                                window_region);

          /*
           * Subtract regions corresponding to any other config windows we
           * might be showing.
           */
          combine_config_windows_input_shape (manager, xdpy, comb_region);

          mutter_plugin_set_stage_input_region (plugin, comb_region);

          XFixesDestroyRegion (xdpy, window_region);
          XFixesDestroyRegion (xdpy, comb_region);

          manager->priv->config_windows =
            g_list_prepend (wins,
                            GINT_TO_POINTER (*config_xwin));

          gtk_widget_show_all (config);

          g_signal_connect (config_socket, "plug-removed",
                            G_CALLBACK (config_plug_removed_cb), child);

          g_signal_connect (config_socket, "size-allocate",
                            G_CALLBACK (config_socket_size_allocate_cb), child);
        }

      XFree (config_xwin);
    }
  else
    gtk_widget_show_all (child->config);

    return TRUE;
}

static void
na_tray_icon_added (NaTrayManager *na_manager, GtkWidget *socket,
                    gpointer user_data)
{
  ShellTrayManager *manager = user_data;
  GtkWidget *win;
  ClutterActor *icon;
  ShellTrayManagerChild *child;
  GdkPixmap *bg_pixmap;

  win = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_container_add (GTK_CONTAINER (win), socket);

  /* The colormap of the socket matches that of its contents; make
   * the window we put it in match that as well */
  gtk_widget_set_colormap (win, gtk_widget_get_colormap (socket));

  gtk_widget_set_size_request (win, 24, 24);
  gtk_widget_realize (win);

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

  gtk_widget_set_parent_window (win, manager->priv->stage_window);
  gdk_window_reparent (win->window, manager->priv->stage_window, 0, 0);
  gtk_window_move (GTK_WINDOW (win), -200, -200);
  gtk_widget_show_all (win);

  icon = clutter_glx_texture_pixmap_new_with_window (GDK_WINDOW_XWINDOW (win->window));
  clutter_x11_texture_pixmap_set_automatic (CLUTTER_X11_TEXTURE_PIXMAP (icon), TRUE);
  clutter_actor_set_size (icon, 24, 24);
  clutter_actor_set_reactive (icon, TRUE);

  child = g_slice_new (ShellTrayManagerChild);
  child->window = win;
  child->socket = socket;
  child->actor = g_object_ref (icon);
  child->manager = manager;
  child->config = NULL;

  g_hash_table_insert (manager->priv->icons, socket, child);

  g_signal_emit (manager,
                 shell_tray_manager_signals[TRAY_ICON_ADDED], 0,
                 icon);

  g_signal_connect (icon, "button-press-event",
                    G_CALLBACK (actor_clicked), child);
}

static void
na_tray_icon_removed (NaTrayManager *na_manager, GtkWidget *socket,
                      gpointer user_data)
{
  ShellTrayManager *manager = user_data;
  ShellTrayManagerChild *child;

  child = g_hash_table_lookup (manager->priv->icons, socket);
  g_return_if_fail (child != NULL);

  g_signal_emit (manager,
                 shell_tray_manager_signals[TRAY_ICON_REMOVED], 0,
                 child->actor);
  g_hash_table_remove (manager->priv->icons, socket);
}

static XserverRegion
create_input_shape (MutterPlugin *plugin,
                    gint x, gint y, gint width, gint height)
{
  XserverRegion  dst, src2;
  XRectangle     r;
  Display       *xdpy = mutter_plugin_get_xdisplay (plugin);

  r.x = 0;
  r.y = 0;
  r.width = width;
  r.height = height;

  src2 = XFixesCreateRegion (xdpy, &r, 1);

  dst = XFixesCreateRegion (xdpy, NULL, 0);

  XFixesSubtractRegion (xdpy, dst,
                        MOBLIN_NETBOOK_PLUGIN (plugin)->priv->screen_region,
                        src2);

  XFixesDestroyRegion (xdpy, src2);

  return dst;
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

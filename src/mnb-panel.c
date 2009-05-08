/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel.c */
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/*
 * MnbDropDown subclass for panels that live in external processes.
 */
#include "mnb-panel.h"
#include "marshal.h"

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

G_DEFINE_TYPE (MnbPanel, mnb_panel, MNB_TYPE_DROP_DOWN)

#define MNB_PANEL_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PANEL, MnbPanelPrivate))

static void     mnb_panel_constructed    (GObject *self);
static void     mnb_panel_show_begin     (MnbDropDown *self);
static void     mnb_panel_show_completed (MnbDropDown *self);
static void     mnb_panel_hide_begin     (MnbDropDown *self);
static void     mnb_panel_hide_completed (MnbDropDown *self);
static gboolean mnb_panel_setup_proxy    (MnbPanel *panel);

enum
{
  PROP_0,

  PROP_DBUS_PATH,
  PROP_WIDTH,
  PROP_HEIGHT,
};

enum
{
  REQUEST_ICON,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _MnbPanelPrivate
{
  DBusGConnection *dbus_conn;
  DBusGProxy      *proxy;
  gchar           *dbus_path;

  gchar           *name;
  gchar           *tooltip;
  guint            xid;
  guint            child_xid;

  GtkWidget       *window;

  guint            width;
  guint            height;

  MutterWindow    *mcw;

  gboolean         constructed : 1; /* poor man's constructor return value. */
};

static void
mnb_panel_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  MnbPanelPrivate *priv = MNB_PANEL (object)->priv;

  switch (property_id)
    {
    case PROP_DBUS_PATH:
      g_value_set_string (value, priv->dbus_path);
      break;
    case PROP_WIDTH:
      g_value_set_uint (value, priv->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, priv->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_panel_set_property (GObject *object, guint property_id,
                        const GValue *value, GParamSpec *pspec)
{
  MnbPanelPrivate *priv = MNB_PANEL (object)->priv;

  switch (property_id)
    {
    case PROP_DBUS_PATH:
      g_free (priv->dbus_path);
      priv->dbus_path = g_value_dup_string (value);
      break;
    case PROP_WIDTH:
      priv->width = g_value_get_uint (value);
      break;
    case PROP_HEIGHT:
      priv->height = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

/*
 * Callbacks for the request signals exposed by the panels.
 */
static void
mnb_panel_request_focus_cb (DBusGProxy *proxy, MnbPanel *panel)
{
  /* TODO */
  g_warning ("%s is not implemented", __FUNCTION__);
}

static void
mnb_panel_request_show_cb (DBusGProxy *proxy, MnbPanel *panel)
{
  clutter_actor_show (CLUTTER_ACTOR (panel));
}

static void
mnb_panel_request_hide_cb (DBusGProxy *proxy, MnbPanel *panel)
{
  clutter_actor_hide (CLUTTER_ACTOR (panel));
}

static void
mnb_panel_launch_application_cb (DBusGProxy  *proxy,
                                 const gchar *app,
                                 gint         workspace,
                                 gboolean     without_chooser,
                                 MnbPanel    *panel)
{
  /* TODO */
  g_warning ("%s is not implemented", __FUNCTION__);
}

static void
mnb_panel_dispose (GObject *self)
{
  MnbPanelPrivate *priv   = MNB_PANEL (self)->priv;
  DBusGProxy      *proxy  = priv->proxy;
  GtkWidget       *window = priv->window;

  if (priv->window)
    {
      priv->window = NULL;
      gtk_widget_destroy (window);
    }

  if (proxy)
    {
      dbus_g_proxy_disconnect_signal (proxy, "RequestShow",
                                      G_CALLBACK (mnb_panel_request_show_cb),
                                      self);

      dbus_g_proxy_disconnect_signal (proxy, "RequestHide",
                                      G_CALLBACK (mnb_panel_request_hide_cb),
                                      self);

      dbus_g_proxy_disconnect_signal (proxy, "RequestFocus",
                                      G_CALLBACK (mnb_panel_request_focus_cb),
                                      self);

      dbus_g_proxy_disconnect_signal (proxy, "LaunchApplication",
                                   G_CALLBACK (mnb_panel_launch_application_cb),
                                   self);

      g_object_unref (proxy);
      priv->proxy = NULL;
    }

  if (priv->dbus_conn)
    {
      dbus_g_connection_unref (priv->dbus_conn);
      priv->dbus_conn = NULL;
    }

  G_OBJECT_CLASS (mnb_panel_parent_class)->dispose (self);
}

static void
mnb_panel_finalize (GObject *object)
{
  MnbPanelPrivate *priv = MNB_PANEL (object)->priv;

  g_free (priv->dbus_path);
  g_free (priv->name);
  g_free (priv->tooltip);

  G_OBJECT_CLASS (mnb_panel_parent_class)->finalize (object);
}

#include "mnb-panel-dbus-bindings.h"

static void
mnb_panel_show (ClutterActor *actor)
{
  MnbPanelPrivate *priv = MNB_PANEL (actor)->priv;

  if (!priv->mcw)
    {
      /*
       * Instead of showing the actor here, we show the associated window.  This
       * triggers call to mnb_panel_show_mutter_window() which then takes care
       * of the actual chaining up to our parent show().
       */
      if (priv->window)
        gtk_widget_show_all (priv->window);
    }
  else
    CLUTTER_ACTOR_CLASS (mnb_panel_parent_class)->show (actor);
}

static gboolean
mnb_panel_dbus_init_panel (MnbPanel  *self,
                           guint      width,
                           guint      height,
                           gchar    **name,
                           guint     *xid,
                           gchar    **tooltip,
                           GError   **error)
{
  MnbPanelPrivate *priv = self->priv;

  return com_intel_Mnb_Panel_init_panel (priv->proxy, width, height, name, xid,
                                         tooltip, error);
}

static gboolean
mnb_panel_dbus_show_begin (MnbPanel *self, GError **error)
{
  MnbPanelPrivate *priv = self->priv;

  if (! com_intel_Mnb_Panel_show_begin (priv->proxy, error))
    {
      g_warning ("ShowBegin failed");
      return FALSE;
    }

  return TRUE;
}

static gboolean
mnb_panel_dbus_show_end (MnbPanel *self, GError **error)
{
  MnbPanelPrivate *priv = self->priv;

  return com_intel_Mnb_Panel_show_end (priv->proxy, error);
}

static gboolean
mnb_panel_dbus_hide_begin (MnbPanel *self, GError **error)
{
  MnbPanelPrivate *priv = self->priv;

  return com_intel_Mnb_Panel_hide_begin (priv->proxy, error);
}

static gboolean
mnb_panel_dbus_hide_end (MnbPanel *self, GError **error)
{
  MnbPanelPrivate *priv = self->priv;

  return com_intel_Mnb_Panel_hide_end (priv->proxy, error);
}

static void
mnb_panel_class_init (MnbPanelClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class    = CLUTTER_ACTOR_CLASS (klass);
  MnbDropDownClass  *dropdown_class = MNB_DROP_DOWN_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbPanelPrivate));

  object_class->get_property     = mnb_panel_get_property;
  object_class->set_property     = mnb_panel_set_property;
  object_class->dispose          = mnb_panel_dispose;
  object_class->finalize         = mnb_panel_finalize;
  object_class->constructed      = mnb_panel_constructed;

  actor_class->show              = mnb_panel_show;

  dropdown_class->show_begin     = mnb_panel_show_begin;
  dropdown_class->show_completed = mnb_panel_show_completed;
  dropdown_class->hide_begin     = mnb_panel_hide_begin;
  dropdown_class->hide_completed = mnb_panel_hide_completed;

  g_object_class_install_property (object_class,
                                   PROP_DBUS_PATH,
                                   g_param_spec_string ("dbus-path",
                                                        "Dbus path",
                                                        "Dbus path",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_WIDTH,
                                   g_param_spec_uint ("width",
                                                      "Width",
                                                      "Width",
                                                      0, G_MAXUINT,
                                                      1024,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_HEIGHT,
                                   g_param_spec_uint ("height",
                                                      "Height",
                                                      "Height",
                                                      0, G_MAXUINT,
                                                      1024,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));


  signals[REQUEST_ICON] =
    g_signal_new ("request-icon",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClass, request_icon),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

static void
mnb_panel_init (MnbPanel *self)
{
  MnbPanelPrivate *priv;

  priv = self->priv = MNB_PANEL_GET_PRIVATE (self);
}

/*
 * Signal closures for the MnbDropDown signals; we translate these into the
 * appropriate dbus method calls.
 */
static void
mnb_panel_show_begin (MnbDropDown *self)
{
  GError *error = NULL;

  if (!mnb_panel_dbus_show_begin (MNB_PANEL (self), &error))
    {
      if (error)
        {
          g_warning ("%s: %s", __FUNCTION__, error->message);
          g_error_free (error);
        }
      else
        g_warning ("%s: Unknown error.", __FUNCTION__);
    }
}

static void
mnb_panel_show_completed (MnbDropDown *self)
{
  MnbPanelPrivate *priv  = MNB_PANEL (self)->priv;
  GError          *error = NULL;
  gfloat           x = 0, y = 0;

  clutter_actor_get_position (CLUTTER_ACTOR (self), &x, &y);

  gtk_window_move (GTK_WINDOW (priv->window), (gint)x, (gint)y);

  if (!mnb_panel_dbus_show_end (MNB_PANEL (self), &error))
    {
      if (error)
        {
          g_warning ("%s: %s", __FUNCTION__, error->message);
          g_error_free (error);
        }
      else
        g_warning ("%s: Unknown error.", __FUNCTION__);
    }
}

static void
mnb_panel_hide_begin (MnbDropDown *self)
{
  GError *error = NULL;

  if (!mnb_panel_dbus_hide_begin (MNB_PANEL (self), &error))
    {
      if (error)
        {
          g_warning ("%s: %s", __FUNCTION__, error->message);
          g_error_free (error);
        }
      else
        g_warning ("%s: Unknown error.", __FUNCTION__);
    }
}

static void
mnb_panel_hide_completed (MnbDropDown *self)
{
  MnbPanelPrivate *priv = MNB_PANEL (self)->priv;
  GError          *error = NULL;
  GtkWidget       *window = priv->window;

  if (!mnb_panel_dbus_hide_end (MNB_PANEL (self), &error))
    {
      if (error)
        {
          g_warning ("%s: %s", __FUNCTION__, error->message);
          g_error_free (error);
        }
      else
        g_warning ("%s: Unknown error.", __FUNCTION__);
    }

  /*
   * We do not hide the window here, only move it off screen; this significantly
   * improves the Toolbar button response time.
   */
  gtk_window_move (GTK_WINDOW (window), 0, -(priv->height + 100));
}

static DBusGConnection *
mnb_panel_connect_to_dbus ()
{
  DBusGConnection *conn;
  GError          *error = NULL;

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (!conn)
    {
      g_warning ("Cannot connect to DBus: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  return conn;
}

static void
mnb_panel_socket_size_allocate_cb (GtkWidget     *widget,
                                   GtkAllocation *allocation,
                                   gpointer       data)
{
  GtkSocket *socket = GTK_SOCKET (widget);

  if (!socket->is_mapped)
    {
      g_debug ("socket deallocated");
    }
  else
    {
      MnbPanelPrivate *priv = MNB_PANEL (data)->priv;
      gfloat x = 0, y = 0;

      g_debug ("socket allocated %d,%d;%dx%d",
               allocation->x,
               allocation->y,
               allocation->width,
               allocation->height);

      clutter_actor_get_position (CLUTTER_ACTOR (data), &x, &y);

      gtk_window_move (GTK_WINDOW (priv->window), (gint)x, (gint)y);
    }
}

static void
mnb_panel_dbus_proxy_weak_notify_cb (gpointer data, GObject *object)
{
  MnbPanel        *panel = MNB_PANEL (data);
  MnbPanelPrivate *priv  = panel->priv;
  gchar           *name;

  g_debug ("Panel died; trying to restart");

  priv->proxy = NULL;

  name = g_strdup (mnb_panel_get_name (panel));

  /*
   * The panel probably crashed; try to restart it, if we fail, just
   * destroy this panel.
   */
  if (!mnb_panel_setup_proxy (panel))
    {
      g_warning ("Unable to restart Panel process '%s'.", name);
      clutter_actor_destroy (CLUTTER_ACTOR (data));
    }
}

static gboolean
mnb_panel_init_owner (MnbPanel *panel)
{
  MnbPanelPrivate *priv = panel->priv;
  guint            xid;
  gchar           *name;
  gchar           *tooltip;
  GError          *error = NULL;
  GtkWidget       *socket;
  GtkWidget       *window;

  /*
   * Now call the remote init_panel() method to obtain the panel name, tooltip
   * and xid.
   */
  if (!mnb_panel_dbus_init_panel (panel, priv->width, priv->height,
                                  &name, &xid, &tooltip, &error))
    {
      g_critical ("Panel initialization for %s failed!",
                  priv->dbus_path);

      if (error)
        {
          g_critical ("%s", error->message);
          g_error_free (error);
        }

      return FALSE;
    }


  if (priv->name)
    g_free (priv->name);

  priv->name = name;

  if (priv->tooltip)
    g_free (priv->tooltip);

  priv->tooltip = tooltip;

  priv->child_xid = xid;

  if (priv->window)
    gtk_widget_destroy (priv->window);

  socket = gtk_socket_new ();
  priv->window = window = gtk_window_new (GTK_WINDOW_POPUP);

  gtk_window_resize (GTK_WINDOW (window), priv->width, priv->height);
  gtk_window_move (GTK_WINDOW (window), 0, 64);

  gtk_container_add (GTK_CONTAINER (window), socket);
  gtk_widget_realize (socket);
  gtk_socket_add_id (GTK_SOCKET (socket), xid);

  if (!GTK_SOCKET (socket)->is_mapped)
    g_warning ("Socket is not mapped !!!");

  g_signal_connect (socket, "size-allocate",
                    G_CALLBACK (mnb_panel_socket_size_allocate_cb),
                    panel);

  gtk_widget_realize (window);

  priv->xid = GDK_WINDOW_XWINDOW (window->window);
}

static void
mnb_panel_proxy_owner_changed_cb (DBusGProxy *proxy,
                                  const char *name,
                                  const char *old,
                                  const char *new,
                                  gpointer    data)
{
  g_debug ("Proxy %s owner changed (old %, new %s).",
           name, old, new);

  mnb_panel_init_owner (MNB_PANEL (data));
}

static gboolean
mnb_panel_setup_proxy (MnbPanel *panel)
{
  MnbPanelPrivate *priv = panel->priv;
  DBusGProxy      *proxy;
  gchar           *dbus_name;

  /*
   * Set up the proxy to the remote object; we mandate that the remote object
   * name must match the provided path exactly, except for the '/' being
   * replaced with '.'. The object must implement the com.intel.Mnb.Panel
   * interface.
   */
  dbus_name = g_strdup (priv->dbus_path + 1);

  g_strdelimit (dbus_name, "/", '.');

  proxy = dbus_g_proxy_new_for_name (priv->dbus_conn,
                                     dbus_name,
                                     priv->dbus_path,
                                     "com.intel.Mnb.Panel");

  g_free (dbus_name);

  if (!proxy)
    {
      g_warning ("Unable to create proxy for % (reason unknown)",
                 priv->dbus_path);

      return FALSE;
    }

  priv->proxy = proxy;

  g_object_weak_ref (G_OBJECT (proxy),
                     mnb_panel_dbus_proxy_weak_notify_cb, panel);

  /*
   * Hook up to the signals the interface defines.
   */
  dbus_g_proxy_add_signal (proxy, "RequestShow", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestShow",
                               G_CALLBACK (mnb_panel_request_show_cb),
                               panel, NULL);

  dbus_g_proxy_add_signal (proxy, "RequestHide", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestHide",
                               G_CALLBACK (mnb_panel_request_hide_cb),
                               panel, NULL);

  dbus_g_proxy_add_signal (proxy, "RequestFocus", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestFocus",
                               G_CALLBACK (mnb_panel_request_focus_cb),
                               panel, NULL);

  dbus_g_object_register_marshaller (
                               moblin_netbook_marshal_VOID__STRING_INT_BOOLEAN,
                               G_TYPE_NONE,
                               G_TYPE_STRING,
                               G_TYPE_INT,
                               G_TYPE_BOOLEAN,
                               G_TYPE_INVALID);

  dbus_g_proxy_add_signal (proxy, "LaunchApplication",
                           G_TYPE_STRING,
                           G_TYPE_INT,
                           G_TYPE_BOOLEAN,
                           G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (proxy, "LaunchApplication",
                               G_CALLBACK (mnb_panel_launch_application_cb),
                               panel, NULL);

  dbus_g_proxy_add_signal (proxy, "NameOwnerChanged",
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (proxy, "NameOwnerChanged",
                               G_CALLBACK (mnb_panel_proxy_owner_changed_cb),
                               panel, NULL);



  return mnb_panel_init_owner (panel);
}

static void
mnb_panel_constructed (GObject *self)
{
  MnbPanelPrivate *priv = MNB_PANEL (self)->priv;
  DBusGConnection *conn;

  /*
   * Make sure our parent gets chance to do what it needs to.
   */
  if (G_OBJECT_CLASS (mnb_panel_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_panel_parent_class)->constructed (self);

  if (!priv->dbus_path)
    return;

  conn = mnb_panel_connect_to_dbus ();

  if (!conn)
    return;

  priv->dbus_conn = conn;

  if (!mnb_panel_setup_proxy (MNB_PANEL (self)))
    return;

  /*
   * Set the constructed flag, so we can check everything went according to
   * plan.
   */
  priv->constructed = TRUE;
}

MnbPanel *
mnb_panel_new (MutterPlugin *plugin,
               const gchar  *dbus_path,
               guint         width,
               guint         height)
{
  MnbPanel *panel = g_object_new (MNB_TYPE_PANEL,
                                  "mutter-plugin", plugin,
                                  "dbus-path",     dbus_path,
                                  "width",         width,
                                  "height",        height,
                                  NULL);

  if (panel && !panel->priv->constructed)
    {
      g_warning ("Panel initialization failed.");
      g_object_unref (panel);
      return NULL;
    }

  return panel;
}

const gchar *
mnb_panel_get_name (MnbPanel *panel)
{
  MnbPanelPrivate *priv = panel->priv;

  return priv->name;
}

const gchar *
mnb_panel_get_tooltip (MnbPanel *panel)
{
  MnbPanelPrivate *priv = panel->priv;

  return priv->tooltip;
}

static void
mnb_panel_mutter_window_destroy_cb (ClutterActor *actor, gpointer data)
{
  MnbPanelPrivate *priv = MNB_PANEL (data)->priv;

  priv->mcw = NULL;

  clutter_actor_hide (CLUTTER_ACTOR (data));
}

void
mnb_panel_show_mutter_window (MnbPanel *panel, MutterWindow *mcw)
{
  MnbPanelPrivate *priv    = panel->priv;
  ClutterActor    *texture;
  Window           xwin;

  if (!mcw)
    {
      if (priv->mcw)
        {
          g_signal_handlers_disconnect_by_func (priv->mcw,
                                           mnb_panel_mutter_window_destroy_cb,
                                           panel);

          priv->mcw = NULL;
        }

      mnb_drop_down_set_child (MNB_DROP_DOWN (panel), NULL);
      return;
    }

  if (mcw == priv->mcw)
    {
      g_debug ("Window already handled.");
      return;
    }

  texture = mutter_window_get_texture (mcw);

  priv->mcw = mcw;

  g_object_ref (texture);
  clutter_actor_unparent (texture);
  mnb_drop_down_set_child (MNB_DROP_DOWN (panel), texture);
  g_object_unref (texture);

  g_signal_connect (mcw, "destroy",
                    G_CALLBACK (mnb_panel_mutter_window_destroy_cb),
                    panel);

  g_object_set (mcw, "no-shadow", TRUE, NULL);

  clutter_actor_hide (CLUTTER_ACTOR (mcw));

  CLUTTER_ACTOR_CLASS (mnb_panel_parent_class)->show (CLUTTER_ACTOR (panel));
}

guint
mnb_panel_get_xid (MnbPanel *panel)
{
  return panel->priv->xid;
}


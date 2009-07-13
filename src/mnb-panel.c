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
#include <moblin-panel/mpl-panel-common.h>

G_DEFINE_TYPE (MnbPanel, mnb_panel, MNB_TYPE_DROP_DOWN)

#define MNB_PANEL_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PANEL, MnbPanelPrivate))

static void     mnb_panel_constructed    (GObject *self);
static void     mnb_panel_show_begin     (MnbDropDown *self);
static void     mnb_panel_show_completed (MnbDropDown *self);
static void     mnb_panel_hide_begin     (MnbDropDown *self);
static void     mnb_panel_hide_completed (MnbDropDown *self);
static gboolean mnb_panel_setup_proxy    (MnbPanel *panel);
static void     mnb_panel_init_owner     (MnbPanel *panel);
static void     mnb_panel_dbus_proxy_weak_notify_cb (gpointer, GObject *);

enum
{
  PROP_0,

  PROP_DBUS_NAME,
  PROP_WIDTH,
  PROP_HEIGHT,
};

enum
{
  READY,
  REQUEST_BUTTON_STYLE,
  REQUEST_TOOLTIP,
  REMOTE_PROCESS_DIED,
  SET_SIZE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _MnbPanelPrivate
{
  DBusGConnection *dbus_conn;
  DBusGProxy      *proxy;
  gchar           *dbus_name;

  gchar           *name;
  gchar           *tooltip;
  gchar           *stylesheet;
  gchar           *button_style_id;
  guint            xid;
  guint            child_xid;

  GtkWidget       *window;

  guint            width;
  guint            height;

  MutterWindow    *mcw;

  gboolean         constructed      : 1;
  gboolean         dead             : 1; /* Set when the remote  */
  gboolean         ready            : 1;
  gboolean         hide_in_progress : 1;
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
    case PROP_DBUS_NAME:
      g_value_set_string (value, priv->dbus_name);
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
    case PROP_DBUS_NAME:
      g_free (priv->dbus_name);
      priv->dbus_name = g_value_dup_string (value);
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
  MnbPanelPrivate *priv;

  if (!CLUTTER_ACTOR_IS_MAPPED (panel))
    {
      g_warning ("Panel %s requested focus while not visible !!!",
                 mnb_panel_get_name (panel));
      return;
    }

  priv = panel->priv;

  XRaiseWindow (GDK_DISPLAY(),
                priv->child_xid);

  XSetInputFocus (GDK_DISPLAY(),
                  priv->child_xid,
                  RevertToPointerRoot,
                  CurrentTime);
}

static void
mnb_panel_request_show_cb (DBusGProxy *proxy, MnbPanel *panel)
{
  clutter_actor_show (CLUTTER_ACTOR (panel));
}

static void
mnb_panel_request_hide_cb (DBusGProxy *proxy, MnbPanel *panel)
{
  mnb_drop_down_hide_with_toolbar (MNB_DROP_DOWN (panel));
}

static void
mnb_panel_request_button_style_cb (DBusGProxy  *proxy,
                                   const gchar *style_id,
                                   MnbPanel    *panel)
{
  MnbPanelPrivate *priv = panel->priv;

  g_free (priv->button_style_id);
  priv->button_style_id = g_strdup (style_id);

  g_signal_emit (panel, signals[REQUEST_BUTTON_STYLE], 0, style_id);
}

static void
mnb_panel_request_tooltip_cb (DBusGProxy  *proxy,
                              const gchar *tooltip,
                              MnbPanel    *panel)
{
  MnbPanelPrivate *priv = panel->priv;

  g_free (priv->tooltip);
  priv->tooltip = g_strdup (tooltip);

  g_signal_emit (panel, signals[REQUEST_TOOLTIP], 0, tooltip);
}

static void
mnb_panel_set_size_cb (DBusGProxy *proxy,
                       guint       width,
                       guint       height,
                       MnbPanel   *panel)
{
  MnbPanelPrivate *priv = panel->priv;

  if (!priv->window)
    return;

  g_debug (G_STRLOC " Got size change on Panel %s",
           mnb_panel_get_name (panel));

  /*
   * Resize our top-level window to match.
   */
  gtk_window_resize (GTK_WINDOW (priv->window), width, height);

  g_signal_emit (panel, signals[SET_SIZE], 0, width, height);
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
      g_object_weak_unref (G_OBJECT (proxy),
                           mnb_panel_dbus_proxy_weak_notify_cb, self);

      dbus_g_proxy_disconnect_signal (proxy, "RequestShow",
                                      G_CALLBACK (mnb_panel_request_show_cb),
                                      self);

      dbus_g_proxy_disconnect_signal (proxy, "RequestHide",
                                      G_CALLBACK (mnb_panel_request_hide_cb),
                                      self);

      dbus_g_proxy_disconnect_signal (proxy, "RequestFocus",
                                      G_CALLBACK (mnb_panel_request_focus_cb),
                                      self);

      dbus_g_proxy_disconnect_signal (proxy, "RequestButtonStyle",
                                 G_CALLBACK (mnb_panel_request_button_style_cb),
                                      self);

      dbus_g_proxy_disconnect_signal (proxy, "RequestTooltip",
                                      G_CALLBACK (mnb_panel_request_tooltip_cb),
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

  g_free (priv->dbus_name);

  g_free (priv->name);
  g_free (priv->tooltip);
  g_free (priv->stylesheet);
  g_free (priv->button_style_id);

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
        {
          gtk_widget_show_all (priv->window);
        }
    }
  else
    CLUTTER_ACTOR_CLASS (mnb_panel_parent_class)->show (actor);
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
                                   PROP_DBUS_NAME,
                                   g_param_spec_string ("dbus-name",
                                                        "Dbus name",
                                                        "Dbus name",
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


  signals[REQUEST_BUTTON_STYLE] =
    g_signal_new ("request-button-style",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClass, request_button_style),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  signals[REQUEST_TOOLTIP] =
    g_signal_new ("request-tooltip",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClass, request_tooltip),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  signals[READY] =
    g_signal_new ("ready",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClass, ready),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[REMOTE_PROCESS_DIED] =
    g_signal_new ("remote-process-died",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClass, remote_process_died),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[SET_SIZE] =
    g_signal_new ("set-size",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClass, set_size),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__UINT_UINT,
                  G_TYPE_NONE, 2,
                  G_TYPE_UINT,
                  G_TYPE_UINT);

  dbus_g_object_register_marshaller (moblin_netbook_marshal_VOID__UINT_UINT,
                                     G_TYPE_NONE,
                                     G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
}

static void
mnb_panel_init (MnbPanel *self)
{
  MnbPanelPrivate *priv;

  priv = self->priv = MNB_PANEL_GET_PRIVATE (self);
}

/*
 * It is not possible to pass a NULL for the reply function into the
 * autogenerated bindings, as they do not check :/
 */
static void
mnb_panel_dbus_dumb_reply_cb (DBusGProxy *proxy, GError *error, gpointer data)
{
}

/*
 * Signal closures for the MnbDropDown signals; we translate these into the
 * appropriate dbus method calls.
 */
static void
mnb_panel_show_begin (MnbDropDown *self)
{
  MnbPanelPrivate *priv = MNB_PANEL (self)->priv;

  org_moblin_UX_Shell_Panel_show_begin_async (priv->proxy,
                                              mnb_panel_dbus_dumb_reply_cb, NULL);
}

static void
mnb_panel_show_completed (MnbDropDown *self)
{
  MnbPanelPrivate *priv  = MNB_PANEL (self)->priv;
  gfloat           x = 0, y = 0;

  clutter_actor_get_position (CLUTTER_ACTOR (self), &x, &y);

  gtk_window_move (GTK_WINDOW (priv->window), (gint)x, (gint)y);

  XRaiseWindow (GDK_DISPLAY(),
                priv->child_xid);

  XSetInputFocus (GDK_DISPLAY(),
                  priv->child_xid,
                  RevertToPointerRoot,
                  CurrentTime);

  org_moblin_UX_Shell_Panel_show_end_async (priv->proxy,
                                            mnb_panel_dbus_dumb_reply_cb, NULL);
}

static void
mnb_panel_hide_begin (MnbDropDown *self)
{
  MnbPanelPrivate *priv = MNB_PANEL (self)->priv;
  GtkWidget       *window = priv->window;

  priv->hide_in_progress = TRUE;

  if (!priv->proxy)
    {
      g_warning (G_STRLOC " No DBus proxy!");
      return;
    }

  /*
   * We do not hide the window here, only move it off screen; this significantly
   * improves the Toolbar button response time.
   */
  gtk_window_move (GTK_WINDOW (window), 0, -(priv->height + 100));

  org_moblin_UX_Shell_Panel_hide_begin_async (priv->proxy,
                                              mnb_panel_dbus_dumb_reply_cb, NULL);
}

static void
mnb_panel_hide_completed (MnbDropDown *self)
{
  MnbPanelPrivate *priv = MNB_PANEL (self)->priv;

  priv->hide_in_progress = FALSE;

  if (!priv->proxy)
    {
      g_warning (G_STRLOC " No DBus proxy!");
      return;
    }

  org_moblin_UX_Shell_Panel_hide_end_async (priv->proxy,
                                            mnb_panel_dbus_dumb_reply_cb, NULL);
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

      /*
       * If the window is mapped, and we are not currently in the hide
       * animation, ensure the window position.
       */
      if (!priv->hide_in_progress && CLUTTER_ACTOR_IS_MAPPED (data))
        {
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
}

static void
mnb_panel_dbus_proxy_weak_notify_cb (gpointer data, GObject *object)
{
  MnbPanel        *panel = MNB_PANEL (data);
  MnbPanelPrivate *priv  = panel->priv;

  priv->proxy = NULL;

  g_object_ref (panel);
  priv->ready = FALSE;
  priv->dead = TRUE;
  g_signal_emit (panel, signals[REMOTE_PROCESS_DIED], 0);
  g_object_unref (panel);
}

static gboolean
mnb_panel_plug_removed_cb (GtkSocket *socket, gpointer data)
{
  MnbPanel        *panel = MNB_PANEL (data);
  MnbPanelPrivate *priv = panel->priv;

  priv->child_xid = None;

  g_object_ref (panel);
  priv->ready = FALSE;
  priv->dead = TRUE;
  g_signal_emit (panel, signals[REMOTE_PROCESS_DIED], 0);
  g_object_unref (panel);

  return FALSE;
}

static void
mnb_panel_init_panel_reply_cb (DBusGProxy *proxy,
                               gchar      *name,
                               guint       xid,
                               gchar      *tooltip,
                               gchar      *stylesheet,
                               gchar      *button_style_id,
                               guint       window_width,
                               guint       window_height,
                               GError     *error,
                               gpointer    panel)
{
  MnbPanelPrivate *priv = MNB_PANEL (panel)->priv;
  GtkWidget       *socket;
  GtkWidget       *window;

  if (error)
    {
      g_warning ("Could not initialize Panel %s: %s",
                 mnb_panel_get_name (panel), error->message);
      return;
    }

  /*
   * We duplicate the return values, because we need to be able to replace them
   * and using the originals we would need to use dbus_malloc() later on
   * to set them afresh.
   */
  g_free (priv->name);
  priv->name = g_strdup (name);

  g_free (priv->tooltip);
  priv->tooltip = g_strdup (tooltip);

  g_free (priv->stylesheet);
  priv->stylesheet = g_strdup (stylesheet);

  g_free (priv->button_style_id);
  priv->button_style_id = g_strdup (button_style_id);

  priv->child_xid = xid;

  /*
   * If we already have a window, we are being called because the panel has
   * died on us; we simply destroy the window and start again. (It should be
   * possible to just destroy the socket and reuse the window (after unmapping
   * it, but this is simple and robust.)
   */
  if (priv->window)
    {
      gtk_widget_destroy (priv->window);
    }

  socket = gtk_socket_new ();
  priv->window = window = gtk_window_new (GTK_WINDOW_POPUP);

  gtk_window_resize (GTK_WINDOW (window), window_width, window_height);

  gtk_window_move (GTK_WINDOW (window), 0, 64);

  gtk_container_add (GTK_CONTAINER (window), socket);
  gtk_widget_realize (socket);
  gtk_socket_add_id (GTK_SOCKET (socket), xid);

  g_signal_connect (socket, "size-allocate",
                    G_CALLBACK (mnb_panel_socket_size_allocate_cb), panel);

  g_signal_connect (socket, "plug-removed",
                    G_CALLBACK (mnb_panel_plug_removed_cb), panel);

  gtk_widget_realize (window);

  priv->xid = GDK_WINDOW_XWINDOW (window->window);

  priv->ready = TRUE;
  priv->dead = FALSE;

  dbus_free (name);
  dbus_free (tooltip);
  dbus_free (stylesheet);
  dbus_free (button_style_id);

  g_signal_emit (panel, signals[READY], 0);
}

/*
 * Does the hard work in establishing a connection to the name owner, retrieving
 * the require information, and constructing the socket into which we embed the
 * panel window.
 */
static void
mnb_panel_init_owner (MnbPanel *panel)
{
  MnbPanelPrivate *priv = panel->priv;

  if (!priv->proxy)
    {
      g_warning (G_STRLOC " No DBus proxy!");
      return;
    }

  /*
   * Now call the remote init_panel() method to obtain the panel name, tooltip
   * and xid.
   */
  org_moblin_UX_Shell_Panel_init_panel_async (priv->proxy,
                                              priv->width, priv->height,
                                              mnb_panel_init_panel_reply_cb,
                                              panel);
}

static void
mnb_panel_proxy_owner_changed_cb (DBusGProxy *proxy,
                                  const char *name,
                                  const char *old,
                                  const char *new,
                                  gpointer    data)
{
  g_debug ("Proxy %s owner changed (old %s, new %s).",
           name, old, new);

  mnb_panel_init_owner (MNB_PANEL (data));
}

static gboolean
mnb_panel_setup_proxy (MnbPanel *panel)
{
  MnbPanelPrivate *priv = panel->priv;
  DBusGProxy      *proxy;
  gchar           *dbus_path;
  gchar           *p;

  if (!priv->dbus_conn)
    {
      g_warning (G_STRLOC " No dbus connection, cannot connect to panel!");
      return FALSE;
    }

  g_debug ("Creating proxy for %s, %p",
           priv->dbus_name, panel);

  dbus_path = g_strconcat ("/", priv->dbus_name, NULL);

  p = dbus_path;
  while (*p)
    {
      if (*p == '.')
        *p = '/';

      ++p;
    }

  proxy = dbus_g_proxy_new_for_name (priv->dbus_conn,
                                     priv->dbus_name,
                                     dbus_path,
                                     MPL_PANEL_DBUS_INTERFACE);

  g_free (dbus_path);

  if (!proxy)
    {
      g_warning ("Unable to create proxy for %s (reason unknown)",
                 priv->dbus_name);

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

  dbus_g_proxy_add_signal (proxy, "NameOwnerChanged",
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (proxy, "NameOwnerChanged",
                               G_CALLBACK (mnb_panel_proxy_owner_changed_cb),
                               panel, NULL);

  dbus_g_proxy_add_signal (proxy, "RequestButtonStyle",
                           G_TYPE_STRING, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestButtonStyle",
                               G_CALLBACK (mnb_panel_request_button_style_cb),
                               panel, NULL);

  dbus_g_proxy_add_signal (proxy, "RequestTooltip",
                           G_TYPE_STRING, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestTooltip",
                               G_CALLBACK (mnb_panel_request_tooltip_cb),
                               panel, NULL);

  dbus_g_proxy_add_signal (proxy, "SetSize",
                           G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "SetSize",
                               G_CALLBACK (mnb_panel_set_size_cb),
                               panel, NULL);

  mnb_panel_init_owner (panel);

  return TRUE;
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

  if (!priv->dbus_name)
    return;

  conn = mnb_panel_connect_to_dbus ();

  if (!conn)
    {
      g_warning (G_STRLOC " Unable to connect to DBus!");
      return;
    }

  priv->dbus_conn = conn;

  if (!mnb_panel_setup_proxy (MNB_PANEL (self)))
    return;

  priv->constructed = TRUE;
}

MnbPanel *
mnb_panel_new (MutterPlugin *plugin,
               const gchar  *dbus_name,
               guint         width,
               guint         height)
{
  MnbPanel *panel = g_object_new (MNB_TYPE_PANEL,
                                  "mutter-plugin", plugin,
                                  "dbus-name",     dbus_name,
                                  "width",         width,
                                  "height",        height,
                                  NULL);

  if (!panel->priv->constructed)
    {
      g_warning (G_STRLOC " Construction of Panel for %s failed.", dbus_name);

      clutter_actor_destroy (CLUTTER_ACTOR (panel));
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

const gchar *
mnb_panel_get_stylesheet (MnbPanel *panel)
{
  MnbPanelPrivate *priv = panel->priv;

  return priv->stylesheet;
}

const gchar *
mnb_panel_get_button_style (MnbPanel *panel)
{
  MnbPanelPrivate *priv = panel->priv;

  return priv->button_style_id;
}

static void
mnb_panel_mutter_window_destroy_cb (ClutterActor *actor, gpointer data)
{
  MnbPanelPrivate *priv = MNB_PANEL (data)->priv;

  priv->mcw = NULL;

  clutter_actor_hide (CLUTTER_ACTOR (data));
}

static void
mnb_panel_pixmap_size_notify_cb (GObject    *gobject,
                                 GParamSpec *pspec,
                                 gpointer    data)
{
  clutter_actor_queue_relayout (CLUTTER_ACTOR (data));
}

void
mnb_panel_show_mutter_window (MnbPanel *panel, MutterWindow *mcw)
{
  MnbPanelPrivate *priv    = panel->priv;
  ClutterActor    *texture;

  if (!mcw)
    {
      if (priv->mcw)
        {
          g_signal_handlers_disconnect_by_func (priv->mcw,
                                           mnb_panel_mutter_window_destroy_cb,
                                           panel);

          texture = mutter_window_get_texture (priv->mcw);

          if (texture)
            g_signal_handlers_disconnect_by_func (texture,
                                           mnb_panel_pixmap_size_notify_cb,
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

  g_signal_connect (texture, "notify::pixmap-width",
                    G_CALLBACK (mnb_panel_pixmap_size_notify_cb),
                    panel);
  g_signal_connect (texture, "notify::pixmap-height",
                    G_CALLBACK (mnb_panel_pixmap_size_notify_cb),
                    panel);

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

gboolean
mnb_panel_is_ready (MnbPanel *panel)
{
  return panel->priv->ready;
}

void
mnb_panel_set_size (MnbPanel *panel, guint width, guint height)
{
  MnbPanelPrivate *priv = panel->priv;

  org_moblin_UX_Shell_Panel_set_size_async (priv->proxy, width, height,
                                            mnb_panel_dbus_dumb_reply_cb, NULL);
}


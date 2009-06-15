/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel-client.c */
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

#include "mnb-panel-client.h"
#include "mnb-panel-common.h"

#include "../src/marshal.h"

#include <string.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

G_DEFINE_TYPE (MnbPanelClient, mnb_panel_client, G_TYPE_OBJECT)

#define MNB_PANEL_CLIENT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PANEL_CLIENT, MnbPanelClientPrivate))

static void     mnb_panel_client_constructed (GObject *self);
static gboolean mnb_panel_client_setup_toolbar_proxy (MnbPanelClient *panel);

enum
{
  PROP_0,

  PROP_NAME,
  PROP_TOOLTIP,
  PROP_STYLESHEET,
  PROP_BUTTON_STYLE,
  PROP_XID,

  PROP_TOOLBAR_SERVICE
};

enum
{
  SET_SIZE,
  SHOW_BEGIN,
  SHOW_END,
  HIDE_BEGIN,
  HIDE_END,

  REQUEST_SHOW,
  REQUEST_HIDE,
  REQUEST_FOCUS,
  REQUEST_BUTTON_STYLE,
  REQUEST_TOOLTIP,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _MnbPanelClientPrivate
{
  DBusGConnection *dbus_conn;
  DBusGProxy      *toolbar_proxy;
  DBusGProxy      *dbus_proxy;

  gchar           *name;
  gchar           *tooltip;
  gchar           *stylesheet;
  gchar           *button_style;
  guint            xid;

  guint            max_height;
  guint            requested_height;

  gboolean         constructed     : 1; /*poor man's constructor return value*/
  gboolean         toolbar_service : 1;
};

static void
mnb_panel_client_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MnbPanelClientPrivate *priv = MNB_PANEL_CLIENT (object)->priv;

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_TOOLTIP:
      g_value_set_string (value, priv->tooltip);
      break;
    case PROP_STYLESHEET:
      g_value_set_string (value, priv->stylesheet);
      break;
    case PROP_BUTTON_STYLE:
      g_value_set_string (value, priv->button_style);
      break;
    case PROP_XID:
      g_value_set_uint (value, priv->xid);
      break;
    case PROP_TOOLBAR_SERVICE:
      g_value_set_boolean (value, priv->toolbar_service);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_panel_client_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MnbPanelClientPrivate *priv = MNB_PANEL_CLIENT (object)->priv;

  switch (property_id)
    {
    case PROP_NAME:
      g_free (priv->name);
      priv->name = g_value_dup_string (value);
      break;
    case PROP_TOOLTIP:
      g_free (priv->tooltip);
      priv->tooltip = g_value_dup_string (value);
      break;
    case PROP_STYLESHEET:
      g_free (priv->stylesheet);
      priv->stylesheet = g_value_dup_string (value);
      break;
    case PROP_BUTTON_STYLE:
      g_free (priv->button_style);
      priv->button_style = g_value_dup_string (value);
      break;
    case PROP_XID:
      priv->xid = g_value_get_uint (value);
      break;
    case PROP_TOOLBAR_SERVICE:
      priv->toolbar_service = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_panel_client_dispose (GObject *self)
{
  MnbPanelClientPrivate *priv  = MNB_PANEL_CLIENT (self)->priv;

  if (priv->toolbar_proxy)
    {
      g_object_unref (priv->toolbar_proxy);
      priv->toolbar_proxy = NULL;
    }

  if (priv->dbus_proxy)
    {
      g_object_unref (priv->dbus_proxy);
      priv->dbus_proxy = NULL;
    }

  if (priv->dbus_conn)
    {
      g_object_unref (priv->dbus_conn);
      priv->dbus_conn = NULL;
    }

  G_OBJECT_CLASS (mnb_panel_client_parent_class)->dispose (self);
}

static void
mnb_panel_client_finalize (GObject *object)
{
  MnbPanelClientPrivate *priv = MNB_PANEL_CLIENT (object)->priv;

  g_free (priv->name);
  g_free (priv->tooltip);
  g_free (priv->stylesheet);
  g_free (priv->button_style);

  G_OBJECT_CLASS (mnb_panel_client_parent_class)->finalize (object);
}

/*
 * The functions required by the interface.
 */
static gboolean
mnb_panel_dbus_init_panel (MnbPanelClient  *self,
                           guint            width,
                           guint            height,
                           gchar          **name,
                           guint           *xid,
                           gchar          **tooltip,
                           gchar          **stylesheet,
                           gchar          **button_style,
                           GError         **error)
{
  MnbPanelClientPrivate *priv = self->priv;
  guint real_height;

  g_debug ("%s called", __FUNCTION__);

  if (!priv->xid)
    return FALSE;

  *xid          = priv->xid;
  *name         = g_strdup (priv->name);
  *tooltip      = g_strdup (priv->tooltip);
  *stylesheet   = g_strdup (priv->stylesheet);
  *button_style = g_strdup (priv->button_style);

  priv->max_height = height;

  if (priv->requested_height > 0 && priv->requested_height < height)
    real_height = priv->requested_height;
  else
    {
      g_warning ("Panel requested height %d which is grater than maximum "
                 "allowable height %d",
                 priv->requested_height, height);

      real_height = height;
    }

  g_signal_emit (self, signals[SET_SIZE], 0, width, real_height);

  return TRUE;
}

static gboolean
mnb_panel_dbus_show_begin (MnbPanelClient *self, GError **error)
{
  g_debug ("%s called", __FUNCTION__);
  g_signal_emit (self, signals[SHOW_BEGIN], 0);
  return TRUE;
}

static gboolean
mnb_panel_dbus_show_end (MnbPanelClient *self, GError **error)
{
  g_debug ("%s called", __FUNCTION__);
  g_signal_emit (self, signals[SHOW_END], 0);
  return TRUE;
}

static gboolean
mnb_panel_dbus_hide_begin (MnbPanelClient *self, GError **error)
{
  g_debug ("%s called", __FUNCTION__);
  g_signal_emit (self, signals[HIDE_BEGIN], 0);
  return TRUE;
}

static gboolean
mnb_panel_dbus_hide_end (MnbPanelClient *self, GError **error)
{
  g_debug ("%s called", __FUNCTION__);
  g_signal_emit (self, signals[HIDE_END], 0);
  return TRUE;
}

#include "../src/mnb-panel-dbus-glue.h"

static void
mnb_panel_client_class_init (MnbPanelClientClass *klass)
{
  GObjectClass     *object_class   = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbPanelClientPrivate));

  object_class->get_property     = mnb_panel_client_get_property;
  object_class->set_property     = mnb_panel_client_set_property;
  object_class->dispose          = mnb_panel_client_dispose;
  object_class->finalize         = mnb_panel_client_finalize;
  object_class->constructed      = mnb_panel_client_constructed;

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_mnb_panel_dbus_object_info);

  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "Name",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_TOOLTIP,
                                   g_param_spec_string ("tooltip",
                                                        "Tooltip",
                                                        "Tooltip",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_STYLESHEET,
                                   g_param_spec_string ("stylesheet",
                                                        "Stylesheet",
                                                        "Stylesheet",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_BUTTON_STYLE,
                                   g_param_spec_string ("button-style",
                                                        "Button style",
                                                        "Button style",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_XID,
                                   g_param_spec_uint ("xid",
                                                      "XID",
                                                      "XID",
                                                      0, G_MAXUINT,
                                                      1024,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_TOOLBAR_SERVICE,
                                   g_param_spec_boolean ("toolbar-service",
                                     "Whether toobar service should be enabled",
                                     "Whether toobar service "
                                     "should be enabled",
                                     FALSE,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY));

  signals[SET_SIZE] =
    g_signal_new ("set-size",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, set_size),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__UINT_UINT,
                  G_TYPE_NONE, 2,
                  G_TYPE_UINT,
                  G_TYPE_UINT);

  signals[SHOW_BEGIN] =
    g_signal_new ("show-begin",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, show_begin),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[SHOW_END] =
    g_signal_new ("show-end",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, show_end),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[HIDE_BEGIN] =
    g_signal_new ("hide-begin",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, hide_begin),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[HIDE_END] =
    g_signal_new ("hide-end",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, hide_end),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[REQUEST_SHOW] =
    g_signal_new ("request-show",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, request_show),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[REQUEST_HIDE] =
    g_signal_new ("request-hide",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, request_hide),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[REQUEST_FOCUS] =
    g_signal_new ("request-focus",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, request_focus),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[REQUEST_BUTTON_STYLE] =
    g_signal_new ("request-button-style",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, request_button_style),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  signals[REQUEST_TOOLTIP] =
    g_signal_new ("request-tooltip",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, request_tooltip),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

static void
mnb_panel_client_init (MnbPanelClient *self)
{
  MnbPanelClientPrivate *priv;

  priv = self->priv = MNB_PANEL_CLIENT_GET_PRIVATE (self);
}

static DBusGConnection *
mnb_panel_client_connect_to_dbus (MnbPanelClient *self)
{
  MnbPanelClientPrivate *priv = MNB_PANEL_CLIENT (self)->priv;
  DBusGConnection       *conn;
  DBusGProxy            *proxy;
  GError                *error = NULL;
  gchar                 *dbus_name;
  guint                  status;

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (!conn)
    {
      g_warning ("Cannot connect to DBus: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  proxy = dbus_g_proxy_new_for_name (conn,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  if (!proxy)
    {
      g_object_unref (conn);
      return NULL;
    }

  dbus_name = g_strconcat (MNB_PANEL_DBUS_NAME_PREFIX, priv->name, NULL);

  if (!org_freedesktop_DBus_request_name (proxy,
                                          dbus_name,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &status, &error))
    {
      if (error)
        {
          g_warning ("%s: %s", __FUNCTION__, error->message);
          g_error_free (error);
        }
      else
        {
          g_warning ("%s: Unknown error", __FUNCTION__);
        }

      g_object_unref (conn);
      conn = NULL;
    }

  g_free (dbus_name);
  g_object_unref (proxy);

  return conn;
}

/*
 * Callback for the DBus.NameOwnerChanged signal, used to setup our proxy
 * when the Toolbar service becomes available.
 */
static void
mnb_panel_client_noc_cb (DBusGProxy     *proxy,
                         const gchar    *name,
                         const gchar    *old_owner,
                         const gchar    *new_owner,
                         MnbPanelClient *panel)
{
  MnbPanelClientPrivate *priv;

  /*
   * Unfortunately, we get this for all name owner changes on the bus, so
   * return early.
   */
  if (!name || strcmp (name, MNB_TOOLBAR_DBUS_NAME))
    return;

  priv = MNB_PANEL_CLIENT (panel)->priv;

  if (!new_owner || !*new_owner)
    {
      /*
       * Toolbar died on us ...
       *
       * NB: we should only be connected when there is no owner, so this
       * should not happen to us
       */
      g_debug ("Toolbar gone away, cleaning up");
      priv->toolbar_proxy = NULL;
      return;
    }

  /*
   * First of all, get rid of the old proxy, if we have one.
   */
  if (priv->toolbar_proxy)
    {
      g_debug ("Already have toolbar proxy, cleaning up");
      g_object_unref (priv->toolbar_proxy);
      priv->toolbar_proxy = NULL;
    }

  if (mnb_panel_client_setup_toolbar_proxy (panel))
    {
      /*
       * If we succeeded, we disconnect the signal handler (and reconnect it
       * again in the weak ref handler for the toolbar proxy.
       */
      dbus_g_proxy_disconnect_signal (priv->dbus_proxy, "NameOwnerChanged",
                                      G_CALLBACK (mnb_panel_client_noc_cb),
                                      panel);
    }
}

static void
mnb_panel_client_dbus_toolbar_proxy_weak_notify_cb (gpointer data,
                                                    GObject *object)
{
  MnbPanelClient        *panel = MNB_PANEL_CLIENT (data);
  MnbPanelClientPrivate *priv  = panel->priv;

  priv->toolbar_proxy = NULL;

  dbus_g_proxy_connect_signal (priv->dbus_proxy, "NameOwnerChanged",
                               G_CALLBACK (mnb_panel_client_noc_cb),
                               panel, NULL);

  g_warning ("Toolbar object died on us\n");
}

/*
 * Sets up connection to the toolbar service, if available.
 */
static gboolean
mnb_panel_client_setup_toolbar_proxy (MnbPanelClient *panel)
{
  MnbPanelClientPrivate *priv = panel->priv;
  DBusGProxy            *proxy;
  GError                *error = NULL;

  g_debug ("Setting up toolbar proxy");

  /*
   * Set up the proxy to the remote toolbar object
   *
   * We creating the proxy for name owner allows us to determine if the service
   * is present or not (_new_for_name() always returns a valid proxy object,
   * which might not be of any use, since the Toolbar service cannot be
   * automatically started).
   */
  proxy = dbus_g_proxy_new_for_name_owner (priv->dbus_conn,
                                           MNB_TOOLBAR_DBUS_NAME,
                                           MNB_TOOLBAR_DBUS_PATH,
                                           MNB_TOOLBAR_DBUS_INTERFACE,
                                           &error);

  if (!proxy)
    {
      /*
       * This is not an error as far as we are concerned, but just one of
       * life's realities (e.g., if the panel process is started before the
       * the Toolbar service is available). We simply sit and wait for the
       * service to appear (so intentionally just g_debug and not g_warning).
       */
      if (error)
        {
          g_debug ("Unable to create proxy for " MNB_TOOLBAR_DBUS_PATH ": %s",
                     error->message);
          g_error_free (error);
        }
      else
        g_debug ("Unable to create proxy for " MNB_TOOLBAR_DBUS_PATH ".");

      return FALSE;
    }
  else
    g_debug ("Got a proxy for " MNB_TOOLBAR_DBUS_NAME " -- ready to roll :-)");

  priv->toolbar_proxy = proxy;

  g_object_weak_ref (G_OBJECT (proxy),
                     mnb_panel_client_dbus_toolbar_proxy_weak_notify_cb, panel);

  return TRUE;
}

static void
mnb_panel_client_constructed (GObject *self)
{
  MnbPanelClientPrivate *priv = MNB_PANEL_CLIENT (self)->priv;
  DBusGConnection       *conn;
  gchar                 *dbus_path;

  /*
   * Make sure our parent gets chance to do what it needs to.
   */
  if (G_OBJECT_CLASS (mnb_panel_client_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_panel_client_parent_class)->constructed (self);

  conn = mnb_panel_client_connect_to_dbus (MNB_PANEL_CLIENT (self));

  if (!conn)
    return;

  priv->dbus_conn = conn;

  dbus_path = g_strconcat (MNB_PANEL_DBUS_PATH_PREFIX, priv->name, NULL);
  dbus_g_connection_register_g_object (conn, dbus_path, self);
  g_free (dbus_path);

  if (priv->toolbar_service)
    {
      DBusGProxy *proxy;
      /*
       * Proxy for the DBus object (need to track availability of the Toolbar
       * service.
       */
      proxy = dbus_g_proxy_new_for_name (conn,
                                         DBUS_SERVICE_DBUS,
                                         DBUS_PATH_DBUS,
                                         DBUS_INTERFACE_DBUS);

      if (!proxy)
        {
          g_critical ("Unable to connect to DBus service !!!");
          return;
        }

      priv->dbus_proxy = proxy;

      dbus_g_proxy_add_signal (proxy, "NameOwnerChanged",
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_INVALID);

      if (!mnb_panel_client_setup_toolbar_proxy (MNB_PANEL_CLIENT (self)))
        {
          /*
           * Toolbar service not yet available; connect to the NameOwnerChanged
           * signal and wait.
           */
          dbus_g_proxy_connect_signal (proxy, "NameOwnerChanged",
                                       G_CALLBACK (mnb_panel_client_noc_cb),
                                       self, NULL);
        }
    }


  /*
   * Set the constructed flag, so we can check everything went according to
   * plan.
   */
  priv->constructed = TRUE;
}

MnbPanelClient *
mnb_panel_client_new (guint        xid,
                      const gchar *name,
                      const gchar *tooltip,
                      const gchar *stylesheet,
                      const gchar *button_style)
{
  MnbPanelClient *panel = g_object_new (MNB_TYPE_PANEL_CLIENT,
                                        "xid",          xid,
                                        "name",         name,
                                        "tooltip",      tooltip,
                                        "stylesheet",   stylesheet,
                                        "button-style", button_style,
                                        NULL);

  if (panel && !panel->priv->constructed)
    {
      g_warning ("Panel initialization failed.");
      g_object_unref (panel);
      return NULL;
    }

  return panel;
}

void
mnb_panel_client_request_show (MnbPanelClient *panel)
{
  g_signal_emit (panel, signals[REQUEST_SHOW], 0);
}

void
mnb_panel_client_request_hide (MnbPanelClient *panel)
{
  g_signal_emit (panel, signals[REQUEST_HIDE], 0);
}

void
mnb_panel_client_request_focus (MnbPanelClient *panel)
{
  g_signal_emit (panel, signals[REQUEST_FOCUS], 0);
}

void
mnb_panel_client_request_button_style (MnbPanelClient *panel,
                                       const gchar    *style)
{
  g_signal_emit (panel, signals[REQUEST_BUTTON_STYLE], 0, style);
}

void
mnb_panel_client_request_tooltip (MnbPanelClient *panel,
                                  const gchar    *tooltip)
{
  g_signal_emit (panel, signals[REQUEST_TOOLTIP], 0, tooltip);
}

#include "../src/mnb-toolbar-dbus-bindings.h"

gboolean
mnb_panel_client_launch_application (MnbPanelClient *panel,
                                     const gchar    *path,
                                     gint            workspace,
                                     gboolean        no_chooser)
{
  MnbPanelClientPrivate *priv = panel->priv;
  GError                *error = NULL;

  if (!priv->toolbar_proxy)
    {
      if (!priv->toolbar_service)
        g_warning ("Launching API is not available because toolbar services "
                   "were not enabled at panel construction !!!");
      else
        g_warning ("Launching API is not available.");

      return FALSE;
    }

  if (!org_moblin_Mnb_Toolbar_launch_application (priv->toolbar_proxy,
                                                  path,
                                                  workspace,
                                                  no_chooser,
                                                  &error))
    {
      if (error)
        {
          g_warning ("Could not launch application %s: %s",
                     path, error->message);
          g_error_free (error);
        }
      else
        g_warning ("Could not launch application %s", path);

      return FALSE;
    }

  return TRUE;
}

gboolean
mnb_panel_client_launch_application_from_desktop_file (MnbPanelClient *panel,
                                                       const gchar    *desktop,
                                                       GList          *files,
                                                       gint            wspace,
                                                       gboolean      no_chooser)
{
  MnbPanelClientPrivate *priv = panel->priv;
  GError                *error = NULL;
  GList                 *l;
  gchar                 *arguments = NULL;

  if (!priv->toolbar_proxy)
    {
      if (!priv->toolbar_service)
        g_warning ("Launching API is not available because toolbar services "
                   "were not enabled at panel construction !!!");
      else
        g_warning ("Launching API is not available.");

      return FALSE;
    }

  while (l)
    {
      if (l->data)
        {
          gchar *a = g_strconcat (arguments, " ", l->data, NULL);
          g_free (arguments);
          arguments = a;
        }
      l = l->next;
    }

  if (!org_moblin_Mnb_Toolbar_launch_application_by_desktop_file (
                                                        priv->toolbar_proxy,
                                                        desktop,
                                                        arguments,
                                                        wspace,
                                                        no_chooser,
                                                        &error))
    {
      if (error)
        {
          g_warning ("Could not launch application from desktop file %s: %s",
                     desktop, error->message);
          g_error_free (error);
        }
      else
        g_warning ("Could not launch application from desktop file %s",
                   desktop);

      return FALSE;
    }

  return TRUE;
}

gboolean
mnb_panel_client_launch_default_application_for_uri (MnbPanelClient *panel,
                                                     const gchar    *uri,
                                                     gint            workspace,
                                                     gboolean        no_chooser)
{
  MnbPanelClientPrivate *priv = panel->priv;
  GError                *error = NULL;

  if (!priv->toolbar_proxy)
    {
      if (!priv->toolbar_service)
        g_warning ("Launching API is not available because toolbar services "
                   "were not enabled at panel construction !!!");
      else
        g_warning ("Launching API is not available.");

      return FALSE;
    }

  if (!org_moblin_Mnb_Toolbar_launch_default_application_for_uri (
                                                  priv->toolbar_proxy,
                                                  uri,
                                                  workspace,
                                                  no_chooser,
                                                  &error))
    {
      if (error)
        {
          g_warning ("Could not launch default application for uri %s: %s",
                     uri, error->message);
          g_error_free (error);
        }
      else
        g_warning ("Could not launch default application for %s", uri);

      return FALSE;
    }

  return TRUE;
}

void
mnb_panel_client_set_height (MnbPanelClient *panel, guint height)
{
  MnbPanelClientPrivate *priv = panel->priv;

  priv->requested_height = height;

  /*
   * If we are called prior to the dbus hand shake, we are done here, otherwise
   * call the vfunction to do the actual work.
   *
   * (max_height is set during the dbus handshake)
   */
  if (priv->max_height > 0)
    {
      if (height <= priv->max_height)
        {
          if (MNB_PANEL_CLIENT_CLASS (mnb_panel_client_parent_class)->
              set_height)
            MNB_PANEL_CLIENT_CLASS (mnb_panel_client_parent_class)->
              set_height (panel, height);
        }
      else
        g_warning ("Panel requested height %d which is grater than maximum "
                   "allowable height %d",
                   height, priv->max_height);
    }
}


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
#include "../src/marshal.h"

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

G_DEFINE_TYPE (MnbPanelClient, mnb_panel_client, G_TYPE_OBJECT)

#define MNB_PANEL_CLIENT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PANEL_CLIENT, MnbPanelClientPrivate))

static void mnb_panel_client_constructed (GObject *self);

enum
{
  PROP_0,

  PROP_DBUS_PATH,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_NAME,
  PROP_TOOLTIP,
  PROP_XID,
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
  REQUEST_ICON,
  LAUNCH_APPLICATION,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _MnbPanelClientPrivate
{
  DBusGConnection *dbus_conn;
  DBusGProxy      *proxy;
  gchar           *dbus_path;

  gchar           *name;
  gchar           *tooltip;
  guint            xid;

  guint            width;
  guint            height;

  gboolean         constructed : 1; /* poor man's constructor return value. */
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
    case PROP_DBUS_PATH:
      g_value_set_string (value, priv->dbus_path);
      break;
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_TOOLTIP:
      g_value_set_string (value, priv->tooltip);
      break;
    case PROP_WIDTH:
      g_value_set_uint (value, priv->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, priv->height);
      break;
    case PROP_XID:
      g_value_set_uint (value, priv->xid);
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
    case PROP_DBUS_PATH:
      g_free (priv->dbus_path);
      priv->dbus_path = g_value_dup_string (value);
      break;
    case PROP_NAME:
      g_free (priv->name);
      priv->name = g_value_dup_string (value);
      break;
    case PROP_TOOLTIP:
      g_free (priv->tooltip);
      priv->tooltip = g_value_dup_string (value);
      break;
    case PROP_WIDTH:
      priv->width = g_value_get_uint (value);
      break;
    case PROP_HEIGHT:
      priv->height = g_value_get_uint (value);
      break;
    case PROP_XID:
      priv->xid = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_panel_client_dispose (GObject *self)
{
  MnbPanelClientPrivate *priv  = MNB_PANEL_CLIENT (self)->priv;

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

  g_free (priv->dbus_path);
  g_free (priv->name);
  g_free (priv->tooltip);

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
                           GError         **error)
{
  MnbPanelClientPrivate *priv = self->priv;

  g_debug ("%s called", __FUNCTION__);

  *xid     = priv->xid;
  *name    = g_strdup (priv->name);
  *tooltip = g_strdup (priv->tooltip);

  g_signal_emit (self, signals[SET_SIZE], 0, width, height);

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
                                   PROP_DBUS_PATH,
                                   g_param_spec_string ("dbus-path",
                                                        "Dbus path",
                                                        "Dbus path",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "Name",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_TOOLTIP,
                                   g_param_spec_string ("tooltip",
                                                        "Tooltip",
                                                        "Tooltip",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_WIDTH,
                                   g_param_spec_uint ("width",
                                                      "Width",
                                                      "Width",
                                                      0, G_MAXUINT,
                                                      1024,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_HEIGHT,
                                   g_param_spec_uint ("height",
                                                      "Height",
                                                      "Height",
                                                      0, G_MAXUINT,
                                                      1024,
                                                      G_PARAM_READWRITE));


  g_object_class_install_property (object_class,
                                   PROP_XID,
                                   g_param_spec_uint ("xid",
                                                      "XID",
                                                      "XID",
                                                      0, G_MAXUINT,
                                                      1024,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  signals[SET_SIZE] =
    g_signal_new ("set-size",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
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

  signals[REQUEST_ICON] =
    g_signal_new ("request-icon",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, request_icon),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  signals[LAUNCH_APPLICATION] =
    g_signal_new ("launch-application",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbPanelClientClass, launch_application),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 3,
                  G_TYPE_STRING,
                  G_TYPE_INT,
                  G_TYPE_BOOLEAN);
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

  dbus_name = g_strdup (priv->dbus_path + 1);

  g_strdelimit (dbus_name, "/", '.');

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

static void
mnb_panel_client_constructed (GObject *self)
{
  MnbPanelClientPrivate *priv = MNB_PANEL_CLIENT (self)->priv;
  DBusGConnection       *conn;

  /*
   * Make sure our parent gets chance to do what it needs to.
   */
  if (G_OBJECT_CLASS (mnb_panel_client_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_panel_client_parent_class)->constructed (self);

  if (!priv->dbus_path)
    return;

  conn = mnb_panel_client_connect_to_dbus (MNB_PANEL_CLIENT (self));

  if (!conn)
    return;

  priv->dbus_conn = conn;

  dbus_g_connection_register_g_object (conn, priv->dbus_path, self);

  /*
   * Set the constructed flag, so we can check everything went according to
   * plan.
   */
  priv->constructed = TRUE;
}

MnbPanelClient *
mnb_panel_client_new (const gchar *dbus_path,
                      guint        xid,
                      const gchar *name,
                      const gchar *tooltip)
{
  MnbPanelClient *panel = g_object_new (MNB_TYPE_PANEL_CLIENT,
                                        "dbus-path", dbus_path,
                                        "xid",       xid,
                                        "name",      name,
                                        "tooltip",   tooltip,
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
mnb_panel_client_request_icon (MnbPanelClient *panel, const gchar *icon)
{
  g_signal_emit (panel, signals[REQUEST_ICON], 0, icon);
}

void
mnb_panel_client_launch_application (MnbPanelClient *panel,
                                     const gchar    *app,
                                     gint            workspace,
                                     gboolean        without_chooser)
{
  g_signal_emit (panel, signals[LAUNCH_APPLICATION], 0,
                 app, workspace, without_chooser);
}


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

/*
 * Top level container for out-of-process panels.
 */
#include "mnb-panel-client.h"
#include "../src/marshal.h"

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

G_DEFINE_TYPE (MnbPanelClient, mnb_panel_client, CLUTTER_TYPE_ACTOR)

#define MNB_PANEL_CLIENT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PANEL_CLIENT, MnbPanelClientPrivate))

static void mnb_panel_client_constructed (GObject *self);

enum
{
  PROP_0,

  PROP_DBUS_PATH,
  PROP_WIDTH,
  PROP_HEIGHT,
};

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
  /* MnbPanelClientPrivate *priv = self->priv; */

  return TRUE;
}

static gboolean
mnb_panel_dbus_show_begin (MnbPanelClient *self, GError **error)
{
  /* MnbPanelClientPrivate *priv = self->priv; */

  return TRUE;
}

static gboolean
mnb_panel_dbus_show_end (MnbPanelClient *self, GError **error)
{
  /* MnbPanelClientPrivate *priv = self->priv; */

  return TRUE;
}

static gboolean
mnb_panel_dbus_hide_begin (MnbPanelClient *self, GError **error)
{
  /* MnbPanelClientPrivate *priv = self->priv; */

  return TRUE;
}

static gboolean
mnb_panel_dbus_hide_end (MnbPanelClient *self, GError **error)
{
  /* MnbPanelClientPrivate *priv = self->priv; */

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
                                                        G_PARAM_READABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

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

}

static void
mnb_panel_client_init (MnbPanelClient *self)
{
  MnbPanelClientPrivate *priv;

  priv = self->priv = MNB_PANEL_CLIENT_GET_PRIVATE (self);
}

static DBusGConnection *
mnb_panel_client_connect_to_dbus ()
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

  conn = mnb_panel_client_connect_to_dbus ();

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
mnb_panel_client_new (const gchar  *dbus_path)
{
  MnbPanelClient *panel = g_object_new (MNB_TYPE_PANEL_CLIENT,
                                        "dbus-path",     dbus_path,
                                        NULL);

  if (panel && !panel->priv->constructed)
    {
      g_warning ("Panel initialization failed.");
      g_object_unref (panel);
      return NULL;
    }

  return panel;
}


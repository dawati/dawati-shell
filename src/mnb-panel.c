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

G_DEFINE_TYPE (MnbPanel, mnb_panel, MNB_TYPE_DROP_DOWN)

#define MNB_PANEL_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PANEL, MnbPanelPrivate))

static void mnb_panel_constructed    (GObject *self);
static void mnb_panel_show_begin     (MnbDropDown *self);
static void mnb_panel_show_completed (MnbDropDown *self);
static void mnb_panel_hide_begin     (MnbDropDown *self);
static void mnb_panel_hide_completed (MnbDropDown *self);

enum
{
  PROP_0,

  PROP_DBUS_PATH,
  PROP_WIDTH,
  PROP_HEIGHT,
};

struct _MnbPanelPrivate
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
      g_value_set_UT_uint32 (value, priv->width);
      break;
    case PROP_HEIGHT:
      g_value_set_UT_uint32 (value, priv->height);
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
      priv->width = g_value_get_UT_uint32 (value);
      break;
    case PROP_HEIGHT:
      priv->height = g_value_get_UT_uint32 (value);
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
}

static void
mnb_panel_request_show_cb (DBusGProxy *proxy, MnbPanel *panel)
{
  /* TODO */
}

static void
mnb_panel_request_hide_cb (DBusGProxy *proxy, MnbPanel *panel)
{
  /* TODO */
}

static void
mnb_panel_launch_application_cb (DBusGProxy  *proxy,
                                 const gchar *app,
                                 gint         workspace,
                                 gboolean     without_chooser,
                                 MnbPanel    *panel)
{
  /* TODO */
}

static void
mnb_panel_dispose (GObject *self)
{
  MnbPanelPrivate *priv  = MNB_PANEL (self)->priv;
  DBusGProxy      *proxy = priv->proxy;

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
      g_object_unref (priv->dbus_conn);
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

/*
 * The functions required by the bindings.
 */
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

  return com_intel_Mnb_Panel_show_begin (priv->proxy, error);
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

#include "mnb-panel-dbus-glue.h"

static void
mnb_panel_class_init (MnbPanelClass *klass)
{
  GObjectClass     *object_class   = G_OBJECT_CLASS (klass);
  MnbDropDownClass *dropdown_class = MNB_DROP_DOWN_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbPanelPrivate));

  object_class->get_property     = mnb_panel_get_property;
  object_class->set_property     = mnb_panel_set_property;
  object_class->dispose          = mnb_panel_dispose;
  object_class->finalize         = mnb_panel_finalize;
  object_class->constructed      = mnb_panel_constructed;

  dropdown_class->show_begin     = mnb_panel_show_begin;
  dropdown_class->show_completed = mnb_panel_show_completed;
  dropdown_class->hide_begin     = mnb_panel_hide_begin;
  dropdown_class->hide_completed = mnb_panel_hide_completed;

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

}

static void
mnb_panel_init (MnbPanel *self)
{
  MnbPanelPrivate *priv;
  MnbPanelClass   *klass = MNB_PANEL_GET_CLASS (self);

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

  if (!mnb_dbus_show_begin (MNB_PANEL (self), &error))
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
  GError *error = NULL;

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
  GError *error = NULL;

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
}

static DBusGConnection *
mnb_panel_connect_to_dbus ()
{
  DBusGConnection *conn;
  GError          *error = NULL;
  guint32          request_name_ret;

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
mnb_panel_constructed (GObject *self)
{
  MnbPanelPrivate *priv = MNB_PANEL (self)->priv;
  DBusGConnection *conn;
  DBusGProxy      *proxy;
  guint            width, height;
  guint            xid;
  gchar           *name;
  gchar           *tooltip;
  GError          *error = NULL;

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

  dbus_g_connection_register_g_object (conn, priv->dbus_path, self);

  proxy = dbus_g_proxy_new_for_name (conn,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  if (!proxy)
    return;

  priv->proxy = proxy;

  if (!mnb_panel_init_panel (self, priv->width, priv->height,
                             &name, &xid, &tooltip, &error))
    {
      g_critical ("Panel initialization for %s failed [%s]!",
                  priv->dbus_path);

      if (error)
        {
          g_critical ("%s", error->message);
          g_error_free (error);
        }

      return;
    }

  dbus_g_proxy_add_signal (proxy, "RequestShow", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestShow",
                               G_CALLBACK (mnb_panel_request_show_cb),
                               self, NULL);

  dbus_g_proxy_add_signal (proxy, "RequestHide", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestHide",
                               G_CALLBACK (mnb_panel_request_hide_cb),
                               self, NULL);

  dbus_g_proxy_add_signal (proxy, "RequestFocus", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "RequestFocus",
                               G_CALLBACK (mnb_panel_request_focus_cb),
                               self, NULL);

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
                               self, NULL);

  priv->name    = name;
  priv->tooltip = tooltip;
  priv->xid     = xid;

  /*
   * TODO -- do something with the xid here, see how the tray manager is
   * done.
   */

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

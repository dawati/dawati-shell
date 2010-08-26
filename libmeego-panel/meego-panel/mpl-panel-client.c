/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mpl-panel-client.c */
/*
 * Copyright (c) 2009, 2010 Intel Corp.
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

#include <string.h>
#include <stdlib.h>

#include <glib/gstdio.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>
#include <gdk/gdk.h>
#include <gio/gdesktopappinfo.h>
#include <clutter/x11/clutter-x11.h>

#include "mpl-app-launches-store.h"
#include "mpl-panel-client.h"
#include "mpl-panel-common.h"
#include "marshal.h"
#include "mnb-enum-types.h"

/**
 * SECTION:mpl-panel-client
 * @short_description: Base abstract class for all panels.
 * @Title: MplPanelClient
 *
 * #MplPanelClient is a base abstract class for all Panels.
 */

G_DEFINE_TYPE (MplPanelClient, mpl_panel_client, G_TYPE_OBJECT)

#define MPL_PANEL_CLIENT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_PANEL_CLIENT, MplPanelClientPrivate))

static void     mpl_panel_client_constructed (GObject *self);
static gboolean mpl_panel_client_setup_toolbar_proxy (MplPanelClient *panel);

enum
{
  PROP_0,

  PROP_NAME,
  PROP_TOOLTIP,
  PROP_STYLESHEET,
  PROP_BUTTON_STYLE,
  PROP_XID,
  PROP_TOOLBAR_SERVICE,
  PROP_DELAYED_READY,
  PROP_WINDOWLESS
};

enum
{
  UNLOAD,
  SET_SIZE,
  SET_POSITION,
  SHOW,
  SHOW_BEGIN,
  SHOW_END,
  HIDE,
  HIDE_BEGIN,
  HIDE_END,

  REQUEST_FOCUS,
  REQUEST_BUTTON_STYLE,
  REQUEST_TOOLTIP,
  REQUEST_BUTTON_STATE,
  REQUEST_MODALITY,

  READY,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _MplPanelClientPrivate
{
  DBusGConnection *dbus_conn;
  DBusGProxy      *toolbar_proxy;
  DBusGProxy      *dbus_proxy;

  gchar           *name;
  gchar           *tooltip;
  gchar           *stylesheet;
  gchar           *button_style;
  guint            xid;

  gint             x;
  gint             y;
  guint            width;
  guint            max_height;
  guint            requested_height;
  guint            real_height;

  gboolean         constructed       : 1; /*poor man's constr return value*/
  gboolean         toolbar_service   : 1;
  gboolean         ready_emitted     : 1;
  gboolean         delayed_ready     : 1;
  gboolean         main_loop_running : 1;
  gboolean         windowless        : 1;
};

static void
mpl_panel_client_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MplPanelClientPrivate *priv = MPL_PANEL_CLIENT (object)->priv;

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
    case PROP_DELAYED_READY:
      g_value_set_boolean (value, priv->delayed_ready);
      break;
    case PROP_WINDOWLESS:
      g_value_set_boolean (value, priv->windowless);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_panel_client_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MplPanelClient        *panel = MPL_PANEL_CLIENT (object);
  MplPanelClientPrivate *priv  = panel->priv;

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
    case PROP_DELAYED_READY:
      mpl_panel_client_set_delayed_ready (panel, g_value_get_boolean (value));
      break;
    case PROP_WINDOWLESS:
      priv->windowless = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_panel_client_dispose (GObject *self)
{
  MplPanelClientPrivate *priv  = MPL_PANEL_CLIENT (self)->priv;

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

  G_OBJECT_CLASS (mpl_panel_client_parent_class)->dispose (self);
}

static void
mpl_panel_client_finalize (GObject *object)
{
  MplPanelClientPrivate *priv = MPL_PANEL_CLIENT (object)->priv;

  g_free (priv->name);
  g_free (priv->tooltip);
  g_free (priv->stylesheet);
  g_free (priv->button_style);

  G_OBJECT_CLASS (mpl_panel_client_parent_class)->finalize (object);
}

/*
 * The functions required by the interface.
 */
static gboolean
mnb_panel_dbus_init_panel (MplPanelClient  *self,
                           gint             x,
                           gint             y,
                           guint            width,
                           guint            height,
                           gchar          **name,
                           guint           *xid,
                           gchar          **tooltip,
                           gchar          **stylesheet,
                           gchar          **button_style,
                           guint           *alloc_width,
                           guint           *alloc_height,
                           GError         **error)
{
  MplPanelClientPrivate *priv = self->priv;
  guint real_height = height;

  g_debug ("dbus init: %d,%d;%dx%d", x, y, width, height);

  *name         = g_strdup (priv->name);
  *tooltip      = g_strdup (priv->tooltip);
  *stylesheet   = g_strdup (priv->stylesheet);
  *button_style = g_strdup (priv->button_style);

  if (!priv->windowless)
    {
      priv->x          = x;
      priv->y          = y;
      priv->max_height = height;
      priv->width      = width;

      if (priv->requested_height > 0 && priv->requested_height < height)
        real_height = priv->requested_height;
      else if (priv->requested_height)
        {
          g_debug ("Panel requested height %d which is greater than maximum "
                   "allowable height %d",
                   priv->requested_height, height);
        }

      priv->real_height = real_height;

      *alloc_width  = width;
      *alloc_height = real_height;

      /*
       * Make sure that the window is hidden (the window can be left mapped, if
       * the Toolbar died on us, and then bad things happen).
       */
      mpl_panel_client_hide (self);

      g_signal_emit (self, signals[SET_SIZE], 0, width, real_height);
      g_signal_emit (self, signals[SET_POSITION], 0, x, y);

      /*
       * Only query the xid after the size and position have been set. This
       * allows the subclasses to defer window creation until the initial size
       * is know.
       */
      *xid = priv->xid;

      if (!priv->xid)
        return FALSE;
    }

  if (priv->ready_emitted)
    {
      /*
       * Are getting embedded for a second, or n-th time; emit the ready
       * signal.
       */

      g_signal_emit (self, signals[READY], 0);
    }

  return TRUE;
}

/*
 * The functions required by the interface.
 */
static gboolean
mnb_panel_dbus_set_size (MplPanelClient  *self,
                         guint            width,
                         guint            height,
                         GError         **error)
{
  MplPanelClientPrivate *priv = self->priv;
  guint real_height = height;
  guint old_width = priv->width;
  guint old_height = priv->real_height;

  if (height > 0)
    priv->max_height = height;

  if (width > 0)
    priv->width = width;

  g_debug ("%s called: width %d (%d), height %d (%d)",
           __FUNCTION__, width, priv->width, height, priv->max_height);


  if (priv->requested_height > 0 && priv->requested_height < height)
    real_height = priv->requested_height;
  else if (priv->requested_height)
    {
      g_debug ("Panel requested height %d is greater than maximum "
               "allowable height %d",
               priv->requested_height, height);
    }

  priv->real_height = real_height;

  if (old_width != priv->width || old_height != real_height)
    g_signal_emit (self, signals[SET_SIZE], 0, priv->width, real_height);

  return TRUE;
}

/*
 * The functions required by the interface.
 */
static gboolean
mnb_panel_dbus_set_position (MplPanelClient  *self,
                             gint             x,
                             gint             y,
                             GError         **error)
{
  MplPanelClientPrivate *priv = self->priv;
  guint old_x = priv->x;
  guint old_y = priv->y;

  if (old_x != priv->x || old_y != priv->y)
    g_signal_emit (self, signals[SET_POSITION], 0, x, y);

  return TRUE;
}

static gboolean
mnb_panel_dbus_show (MplPanelClient *self, GError **error)
{
  g_signal_emit (self, signals[SHOW], 0);
  return TRUE;
}

static gboolean
mnb_panel_dbus_show_begin (MplPanelClient *self, GError **error)
{
  g_signal_emit (self, signals[SHOW_BEGIN], 0);
  return TRUE;
}

static gboolean
mnb_panel_dbus_show_end (MplPanelClient *self, GError **error)
{
  g_signal_emit (self, signals[SHOW_END], 0);
  return TRUE;
}

static gboolean
mnb_panel_dbus_hide (MplPanelClient *self, GError **error)
{
  g_signal_emit (self, signals[HIDE], 0);
  return TRUE;
}

static gboolean
mnb_panel_dbus_hide_begin (MplPanelClient *self, GError **error)
{
  g_signal_emit (self, signals[HIDE_BEGIN], 0);
  return TRUE;
}

static gboolean
mnb_panel_dbus_hide_end (MplPanelClient *self, GError **error)
{
  g_signal_emit (self, signals[HIDE_END], 0);
  return TRUE;
}

static gboolean
mnb_panel_dbus_ping (MplPanelClient *self, GError **error)
{
  return TRUE;
}

static gboolean
mnb_panel_dbus_unload (MplPanelClient *self, GError **error)
{
  g_signal_emit (self, signals[UNLOAD], 0);

  return TRUE;
}

#include "mnb-panel-dbus-glue.h"

static void
mpl_panel_client_real_unload (MplPanelClient *panel)
{
  g_main_loop_quit (NULL);
}

static void
mpl_panel_client_class_init (MplPanelClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MplPanelClientPrivate));

  object_class->get_property     = mpl_panel_client_get_property;
  object_class->set_property     = mpl_panel_client_set_property;
  object_class->dispose          = mpl_panel_client_dispose;
  object_class->finalize         = mpl_panel_client_finalize;
  object_class->constructed      = mpl_panel_client_constructed;

  klass->unload                  = mpl_panel_client_real_unload;

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
                                                      0,
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

  g_object_class_install_property (object_class,
                                   PROP_DELAYED_READY,
                                   g_param_spec_boolean ("delayed-ready",
                                     "Delayed emission of 'ready' signal",
                                     "Whether emission of 'ready' signal should"
                                     " be delayed until later.",
                                     FALSE,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_WINDOWLESS,
                                   g_param_spec_boolean ("windowless",
                                     "Panel without window",
                                     "Panel without window",
                                     FALSE,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT));

  /**
   * MplPanelClient::unload:
   * @panel: panel that received the signal.
   *
   * The ::unload signal is emitted when the panel is being unloaded from
   * memory; the actual unloading is done by the signal closure.
   */
  signals[UNLOAD] =
    g_signal_new ("unload",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, unload),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * MplPanelClient::set-size:
   * @panel: panel that received the signal
   * @width: new width of the panel
   * @height: new height of the panel
   *
   * The ::set-size signal is emitted when the panel is being resized; the
   * actual resizing is implemented by the signal closure.
   */
  signals[SET_SIZE] =
    g_signal_new ("set-size",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MplPanelClientClass, set_size),
                  NULL, NULL,
                  meego_netbook_marshal_VOID__UINT_UINT,
                  G_TYPE_NONE, 2,
                  G_TYPE_UINT,
                  G_TYPE_UINT);

  /**
   * MplPanelClient::set-position:
   * @panel: panel that received the signal
   * @x: new x coordinate of the panel
   * @y: new y coordinate of the panel
   *
   * The ::set-position signal is emitted when the panel is being moved; the
   * actual moving of the panel is implemented by the signal closure.
   */
  signals[SET_POSITION] =
    g_signal_new ("set-position",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MplPanelClientClass, set_position),
                  NULL, NULL,
                  meego_netbook_marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT,
                  G_TYPE_INT);

  /**
   * MplPanelClient::show-begin:
   * @panel: panel that received the signal
   *
   * The ::show-begin signal is emitted when the Toolbar is about to start the
   * panel show animation.
   */
  signals[SHOW_BEGIN] =
    g_signal_new ("show-begin",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, show_begin),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * MplPanelClient::show-end:
   * @panel: panel that received the signal
   *
   * The ::show-end signal is emitted when the panel show animation has
   * finished.
   */
  signals[SHOW_END] =
    g_signal_new ("show-end",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, show_end),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * MplPanelClient::hide-begin:
   * @panel: panel that received the signal
   *
   * The ::hide-begin signal is emitted when the Toolbar is about to start the
   * panel hide animation.
   */
  signals[HIDE_BEGIN] =
    g_signal_new ("hide-begin",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, hide_begin),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * MplPanelClient::hide-end:
   * @panel: panel that received the signal
   *
   * The ::hide-end signal is emitted when the panel hide animation has
   * finished.
   */
  signals[HIDE_END] =
    g_signal_new ("hide-end",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, hide_end),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * MplPanelClient::show:
   * @panel: panel that received the signal
   *
   * The ::show signal is emitted when the panel is about to be shown; the
   * actual showing is implemented by the signal closure.
   */
  signals[SHOW] =
    g_signal_new ("show",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, show),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * MplPanelClient::hide:
   * @panel: panel that received the signal
   *
   * The ::show signal is emitted when the panel is about to be hidden; the
   * actual hiding is implemented by the signal closure.
   */
  signals[HIDE] =
    g_signal_new ("hide",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, hide),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /*
   * The following signals are here for the Panel dbus interface
   */
  signals[REQUEST_FOCUS] =
    g_signal_new ("request-focus",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, request_focus),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[REQUEST_BUTTON_STYLE] =
    g_signal_new ("request-button-style",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, request_button_style),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  signals[REQUEST_TOOLTIP] =
    g_signal_new ("request-tooltip",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, request_tooltip),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  signals[REQUEST_BUTTON_STATE] =
    g_signal_new ("request-button-state",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, request_button_state),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT); /* really MNB_TYPE_BUTTON_STATE, but dbus
                                * introspection chokes on enum types
                                */

  signals[REQUEST_MODALITY] =
    g_signal_new ("request-modality",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplPanelClientClass, request_modality),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  signals[READY] =
    g_signal_new ("ready",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mpl_panel_client_init (MplPanelClient *self)
{
  MplPanelClientPrivate *priv;
  const gchar           *home;

  priv = self->priv = MPL_PANEL_CLIENT_GET_PRIVATE (self);

  home = g_get_home_dir ();

  if (g_chdir (home))
    g_warning ("Unable to change working directory to '%s'", home);
}

static DBusGConnection *
mpl_panel_client_connect_to_dbus (MplPanelClient *self)
{
  MplPanelClientPrivate *priv = MPL_PANEL_CLIENT (self)->priv;
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
      exit (-1);
    }

  proxy = dbus_g_proxy_new_for_name (conn,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  if (!proxy)
    {
      g_warning ("Unable to create proxy for %s", DBUS_PATH_DBUS);
      exit (-1);
    }

  dbus_name = g_strconcat (MPL_PANEL_DBUS_NAME_PREFIX, priv->name, NULL);

  if (!org_freedesktop_DBus_request_name (proxy,
                                          dbus_name,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &status, &error) ||
      (status != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER))
    {
      if (error)
        {
          g_warning ("%s: %s", __FUNCTION__, error->message);
          g_error_free (error);
        }

      /*
       * If we cannot acquire dbus name, than we have no reason to live.
       */
      g_warning ("Unable to acquire dbus name, exiting.");
      exit (-1);
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
mpl_panel_client_noc_cb (DBusGProxy     *proxy,
                         const gchar    *name,
                         const gchar    *old_owner,
                         const gchar    *new_owner,
                         MplPanelClient *panel)
{
  MplPanelClientPrivate *priv;

  /*
   * Unfortunately, we get this for all name owner changes on the bus, so
   * return early.
   */
  if (!name || strcmp (name, MPL_TOOLBAR_DBUS_NAME))
    return;

  priv = MPL_PANEL_CLIENT (panel)->priv;

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

      mpl_panel_client_hide (panel);
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

  if (mpl_panel_client_setup_toolbar_proxy (panel))
    {
      /*
       * If we succeeded, we disconnect the signal handler (and reconnect it
       * again in the weak ref handler for the toolbar proxy.
       */
      dbus_g_proxy_disconnect_signal (priv->dbus_proxy, "NameOwnerChanged",
                                      G_CALLBACK (mpl_panel_client_noc_cb),
                                      panel);
    }
}

static void
mpl_panel_client_dbus_toolbar_proxy_weak_notify_cb (gpointer data,
                                                    GObject *object)
{
  MplPanelClient        *panel = MPL_PANEL_CLIENT (data);
  MplPanelClientPrivate *priv  = panel->priv;

  priv->toolbar_proxy = NULL;

  dbus_g_proxy_connect_signal (priv->dbus_proxy, "NameOwnerChanged",
                               G_CALLBACK (mpl_panel_client_noc_cb),
                               panel, NULL);

  g_debug ("Toolbar object died on us\n");
}

/*
 * Sets up connection to the toolbar service, if available.
 */
static gboolean
mpl_panel_client_setup_toolbar_proxy (MplPanelClient *panel)
{
  MplPanelClientPrivate *priv = panel->priv;
  DBusGProxy            *proxy;
  GError                *error = NULL;

  /*
   * Set up the proxy to the remote toolbar object
   *
   * We creating the proxy for name owner allows us to determine if the service
   * is present or not (_new_for_name() always returns a valid proxy object,
   * which might not be of any use, since the Toolbar service cannot be
   * automatically started).
   */
  proxy = dbus_g_proxy_new_for_name_owner (priv->dbus_conn,
                                           MPL_TOOLBAR_DBUS_NAME,
                                           MPL_TOOLBAR_DBUS_PATH,
                                           MPL_TOOLBAR_DBUS_INTERFACE,
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
          g_debug ("Unable to create proxy for " MPL_TOOLBAR_DBUS_PATH ": %s",
                     error->message);
          g_error_free (error);
        }
      else
        g_debug ("Unable to create proxy for " MPL_TOOLBAR_DBUS_PATH ".");

      return FALSE;
    }
  else
    g_debug ("Got a proxy for " MPL_TOOLBAR_DBUS_NAME " -- ready to roll :-)");

  priv->toolbar_proxy = proxy;

  g_object_weak_ref (G_OBJECT (proxy),
                     mpl_panel_client_dbus_toolbar_proxy_weak_notify_cb, panel);

  return TRUE;
}

/*
 * We install a one-off idle to emit the READY signal
 */
static gboolean
mpl_panel_client_ready_idle_cb (gpointer data)
{
  MplPanelClient *panel = data;

  static guint count = 0;

  /*
   * Allow the main loop to spin few times
   */
  if (count++ < 5)
    return TRUE;

  g_signal_emit (panel, signals[READY], 0);

  panel->priv->ready_emitted = TRUE;

  return FALSE;
}

/*
 * We install a one-off idle to install the ready idle, if needed
 */
static gboolean
mpl_panel_client_one_off_idle_cb (gpointer data)
{
  MplPanelClient *panel = data;

  if (!panel->priv->delayed_ready)
    g_idle_add_full (G_PRIORITY_LOW,
                     mpl_panel_client_ready_idle_cb, panel, NULL);

  panel->priv->main_loop_running = TRUE;

  return FALSE;
}

static void
mpl_panel_client_constructed (GObject *self)
{
  MplPanelClientPrivate *priv = MPL_PANEL_CLIENT (self)->priv;
  DBusGConnection       *conn;
  gchar                 *dbus_path;

  /*
   * Make sure our parent gets chance to do what it needs to.
   */
  if (G_OBJECT_CLASS (mpl_panel_client_parent_class)->constructed)
    G_OBJECT_CLASS (mpl_panel_client_parent_class)->constructed (self);

  conn = mpl_panel_client_connect_to_dbus (MPL_PANEL_CLIENT (self));

  if (!conn)
    return;

  priv->dbus_conn = conn;

  dbus_path = g_strconcat (MPL_PANEL_DBUS_PATH_PREFIX, priv->name, NULL);
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

      if (!mpl_panel_client_setup_toolbar_proxy (MPL_PANEL_CLIENT (self)))
        {
          /*
           * Toolbar service not yet available; connect to the NameOwnerChanged
           * signal and wait.
           */
          dbus_g_proxy_connect_signal (proxy, "NameOwnerChanged",
                                       G_CALLBACK (mpl_panel_client_noc_cb),
                                       self, NULL);
        }
    }


  /*
   * Set the constructed flag, so we can check everything went according to
   * plan.
   */
  priv->constructed = TRUE;

  g_idle_add (mpl_panel_client_one_off_idle_cb, self);
}

/**
 * mpl_panel_client_unload:
 * @panel: #MplPanelClient
 *
 * Unloads the given panel.
 */
void
mpl_panel_client_unload (MplPanelClient *panel)
{
  g_signal_emit (panel, signals[UNLOAD], 0);
}

/**
 * mpl_panel_client_show:
 * @panel: #MplPanelClient
 *
 * Shows the given panel.
 */
void
mpl_panel_client_show (MplPanelClient *panel)
{
  g_signal_emit (panel, signals[SHOW], 0);
}

/**
 * mpl_panel_client_hide:
 * @panel: #MplPanelClient
 *
 * Hides the given panel.
 */
void
mpl_panel_client_hide (MplPanelClient *panel)
{
  g_signal_emit (panel, signals[HIDE], 0);
}

/**
 * mpl_panel_client_request_focus:
 * @panel: #MplPanelClient
 *
 * Requests that the given panel be given keyboard focus.
 */
void
mpl_panel_client_request_focus (MplPanelClient *panel)
{
  g_signal_emit (panel, signals[REQUEST_FOCUS], 0);
}

/**
 * mpl_panel_client_request_button_style:
 * @panel: #MplPanelClient
 * @style: css id of the style.
 *
 * Requests that the supplied style is applied to the Toolbar button associated
 * with the given panel.
 */
void
mpl_panel_client_request_button_style (MplPanelClient *panel,
                                       const gchar    *style)
{
  MplPanelClientPrivate *priv = MPL_PANEL_CLIENT (panel)->priv;

  if (!g_str_equal (style, priv->button_style))
    {
      g_free (priv->button_style);
      priv->button_style = g_strdup (style);

      g_signal_emit (panel, signals[REQUEST_BUTTON_STYLE], 0, style);
    }
}

/**
 * mpl_panel_client_request_tooltip:
 * @panel: #MplPanelClient
 * @tooltip: tooltip
 *
 * Requests that the string is used as the tooltip for the Toolbar button
 * associated with the given panel.
 */
void
mpl_panel_client_request_tooltip (MplPanelClient *panel,
                                  const gchar    *tooltip)
{
  MplPanelClientPrivate *priv = MPL_PANEL_CLIENT (panel)->priv;

  g_free (priv->tooltip);
  priv->tooltip = g_strdup (tooltip);

  g_signal_emit (panel, signals[REQUEST_TOOLTIP], 0, tooltip);
}

/**
 * mpl_panel_client_request_button_state:
 * @panel: #MplPanelClient
 * @state: #MnbButtonState
 *
 * Requests that the Toolbar button associated with the given panel is turned
 * into the supplied state.
 */
void
mpl_panel_client_request_button_state (MplPanelClient *panel,
                                       MnbButtonState  state)
{
  g_signal_emit (panel, signals[REQUEST_BUTTON_STATE], 0, state);
}

/**
 * mpl_panel_client_request_modality:
 * @panel: #MplPanelClient
 * @modal: #gboolean indicating whether panel should be modal
 *
 * Requests that the given panel is put into a modal state; while a panel is
 * in modal state, the user will not be able to close it.
 */
void
mpl_panel_client_request_modality (MplPanelClient *panel, gboolean modal)
{
  g_signal_emit (panel, signals[REQUEST_MODALITY], 0, modal);
}

#include "mnb-toolbar-dbus-bindings.h"

/*
 * Helper function to launch application from GAppInfo, with all the
 * required SN housekeeping.
 */
static gboolean
mpl_panel_client_launch_application_from_info (GAppInfo *app, GList *files)
{
  GAppLaunchContext    *ctx;
  GdkAppLaunchContext  *gctx;
  GError               *error = NULL;
  gboolean              retval = TRUE;
  guint32               timestamp;

  gctx = gdk_app_launch_context_new ();
  ctx  = G_APP_LAUNCH_CONTEXT (gctx);

  timestamp = clutter_x11_get_current_event_time ();

  gdk_app_launch_context_set_timestamp (gctx, timestamp);

  retval = g_app_info_launch (app, files, ctx, &error);

  if (!retval)
    {
      if (error)
        {
          g_warning ("Failed to launch %s (%s)",
#if GLIB_CHECK_VERSION(2,20,0)
                     g_app_info_get_commandline (app),
#else
                     g_app_info_get_name (app),
#endif
                     error->message);

          g_error_free (error);
        }
      else
        {
          g_warning ("Failed to launch %s",
#if GLIB_CHECK_VERSION(2,20,0)
                     g_app_info_get_commandline (app)
#else
                     g_app_info_get_name (app)
#endif
                     );
        }
    }
  else
    {
      /* Track app launch */
      MplAppLaunchesStore *store = mpl_app_launches_store_new ();
      char const *executable = g_app_info_get_executable (app);
      mpl_app_launches_store_add_async (store, executable, 0, &error);
      if (error)
      {
        g_warning ("%s : %s", G_STRLOC, error->message);
        g_clear_error (&error);
      }
      g_object_unref (store);
    }

  g_object_unref (ctx);

  return retval;
}

/**
 * mpl_panel_client_launch_application:
 * @panel: #MplPanelClient
 * @commandline: commandline
 *
 * Launches the program specified by commandline with startup notification
 * support.
 *
 * Return value: %TRUE if there was no error.
 */
gboolean
mpl_panel_client_launch_application (MplPanelClient *panel,
                                     const gchar    *commandline)
{
  GAppInfo *app;
  GError   *error = NULL;
  gboolean  retval;
  GKeyFile *key_file;
  gchar    *cmd;

  g_return_val_if_fail (commandline, FALSE);

#if 1
  /*
   * Startup notification only works with the g_app_launch API when we both
   * supply a GDK launch context *and* create the GAppInfo from a desktop file
   * that has the StartupNotify field set to true.
   *
   * To work around the limitations, we fake a desktop file via the key-file
   * API (The alternative is provide a custom GAppInfo impelentation, but the
   * GAppInfo design makes that hard: g_app_info_create_from_commandline () is
   * hardcoded to use GDesktopAppInfo, so for any internal glib paths that
   * pass through it, we are pretty much screwed anyway. It seemed like this
   * 7LOC hack made more sense than the 800LOC, no less hacky, alternative.
   */
  cmd = g_strdup_printf ("%s %%u", commandline);
  key_file = g_key_file_new ();

  g_key_file_set_string  (key_file, G_KEY_FILE_DESKTOP_GROUP,
                          G_KEY_FILE_DESKTOP_KEY_TYPE,
                          G_KEY_FILE_DESKTOP_TYPE_APPLICATION);
  g_key_file_set_string  (key_file, G_KEY_FILE_DESKTOP_GROUP,
                          G_KEY_FILE_DESKTOP_KEY_EXEC, cmd);
  g_key_file_set_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP,
                          G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY, TRUE);

  app = (GAppInfo*)g_desktop_app_info_new_from_keyfile (key_file);

  g_key_file_free (key_file);
  g_free (cmd);
#else
  app = g_app_info_create_from_commandline (path, NULL,
                                            G_APP_INFO_CREATE_SUPPORTS_URIS,
                                            &error);
#endif

  if (error)
    {
      g_warning ("Failed to create GAppInfo from commnad line %s (%s)",
                 commandline, error->message);

      g_error_free (error);
      return FALSE;
    }

  retval = mpl_panel_client_launch_application_from_info (app, NULL);

  g_object_unref (app);

  return retval;
}

/**
 * mpl_panel_client_launch_application_from_desktop_file:
 * @panel: #MplPanelClient
 * @desktop: path to desktop file
 * @files: #GList of files to be passed as arugments, or %NULL
 *
 * Launches the program specified by desktop file.
 *
 * Return value: %TRUE if there was no error.
 */
gboolean
mpl_panel_client_launch_application_from_desktop_file (MplPanelClient *panel,
                                                       const gchar    *desktop,
                                                       GList          *files)
{
  GAppInfo *app;
  gboolean  retval;

  g_return_val_if_fail (desktop, FALSE);

  app = G_APP_INFO (g_desktop_app_info_new_from_filename (desktop));

  if (!app)
    {
      g_warning ("Failed to create GAppInfo for file %s", desktop);
      return FALSE;
    }

  retval = mpl_panel_client_launch_application_from_info (app, files);

  g_object_unref (app);

  return retval;
}

/**
 * mpl_panel_client_launch_default_application_for_uri:
 * @panel: #MplPanelClient
 * @uri: path to desktop file
 *
 * Launches the default program handling the given uri type.
 *
 * Return value: %TRUE if there was no error.
 */
gboolean
mpl_panel_client_launch_default_application_for_uri (MplPanelClient *panel,
                                                     const gchar    *uri)
{
  GAppLaunchContext   *ctx;
  GdkAppLaunchContext *gctx;
  GAppInfo            *app;
  GError              *error = NULL;
  gboolean             retval = TRUE;
  gchar               *uri_scheme;
  guint32              timestamp;

  uri_scheme = g_uri_parse_scheme (uri);

  /* For local files we want the local file handler not the scheme handler */
  if (g_str_equal (uri_scheme, "file"))
    {
      GFile *file;
      file = g_file_new_for_uri (uri);
      app = g_file_query_default_handler (file, NULL, NULL);
      g_object_unref (file);
    }
  else
    {
      app = g_app_info_get_default_for_uri_scheme (uri_scheme);
    }

  g_free (uri_scheme);

  gctx = gdk_app_launch_context_new ();
  ctx  = G_APP_LAUNCH_CONTEXT (gctx);

  timestamp = clutter_x11_get_current_event_time ();

  gdk_app_launch_context_set_timestamp (gctx, timestamp);

  retval = g_app_info_launch_default_for_uri (uri, ctx, &error);

  if (!retval)
    {
      if (error)
        {
          g_warning ("Failed to launch default app for %s (%s)",
                     uri, error->message);

          g_error_free (error);
        }
      else
        g_warning ("Failed to launch default app for %s", uri);
    }

  g_object_unref (ctx);

  return retval;
}

/**
 * mpl_panel_client_set_height_request:
 * @panel: #MplPanelClient
 * @height: height request
 *
 * Sets the height request for given panel; this is the size the panel
 * wishes to be. This is a request only, i.e., the panel can be smaller if the
 * request cannot be honored due to screen size, etc.
 */
void
mpl_panel_client_set_height_request (MplPanelClient *panel, guint height)
{
  MplPanelClientPrivate *priv;

  g_return_if_fail (MPL_IS_PANEL_CLIENT (panel));

  priv = panel->priv;

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
          g_signal_emit (panel, signals[SET_SIZE], 0, priv->width, height);
        }
      else
        g_warning ("Panel requested height %d which is grater than maximum "
                   "allowable height %d",
                   height, priv->max_height);
    }
}

/**
 * mpl_panel_client_get_height_request:
 * @panel: #MplPanelClient
 *
 * Returns the height request previously set with
 * mpl_panel_client_set_height_request().
 *
 * Return value: requested height in pixels.
 */
guint
mpl_panel_client_get_height_request (MplPanelClient *panel)
{
  g_return_val_if_fail (MPL_IS_PANEL_CLIENT (panel), 0);

  return panel->priv->requested_height;
}

/**
 * mpl_panel_client_get_xid:
 * @panel: #MplPanelClient
 *
 * Returns the xid of the asociated window.
 *
 * Return value: associated window xid.
 */
Window
mpl_panel_client_get_xid (MplPanelClient *panel)
{
  g_return_val_if_fail (MPL_IS_PANEL_CLIENT (panel), None);

  return panel->priv->xid;
}

void
mpl_panel_client_request_show (MplPanelClient *panel)
{
  g_warning ("%s is deprecated use mpl_panel_client_show() instead.",
             __FUNCTION__);

  mpl_panel_client_show (panel);
}

void
mpl_panel_client_request_hide (MplPanelClient *panel)
{
  g_warning ("%s is deprecated use mpl_panel_client_hide() instead.",
             __FUNCTION__);

  mpl_panel_client_hide (panel);
}

/**
 * mpl_panel_client_ready:
 * @panel: #MplPanelClient
 *
 * Calling this function indicates to the Toolbar object that the panel is
 * ready to be shown. This function should be used in conjunction with
 * mpl_panel_client_set_delayed_ready().
 */
void
mpl_panel_client_ready (MplPanelClient *panel)
{
  MplPanelClientPrivate *priv;

  g_return_if_fail (MPL_IS_PANEL_CLIENT (panel));

  priv = panel->priv;

  if (!priv->delayed_ready)
    {
      g_warning ("%s can only be called if the client object was constructed "
                 "with 'delayed-ready' property set to TRUE.!",
                 __FUNCTION__);
      return;
    }

  priv->ready_emitted = TRUE;

  g_signal_emit (panel, signals[READY], 0);
}

/**
 * mpl_panel_client_set_delayed_ready:
 * @panel: a #MplPanelClient
 * @delayed: boolean value for the delayed-ready property.
 *
 * Sets the value of the delayed-ready property. When this property is set to
 * %TRUE the #MplPanelClient::ready signal will not be emitted until the panel
 * application calls mpl_panel_client_ready().
 *
 * Note: The delayed-ready property can only be set before the main loop is
 * started; calling this function once the main loop has been started will
 * result in a critical warning.
 *
 * Return value: %TRUE if the call was successful.
 */
gboolean
mpl_panel_client_set_delayed_ready (MplPanelClient *panel, gboolean delayed)
{
  MplPanelClientPrivate *priv;
  gboolean               not_delayed;

  g_return_val_if_fail (MPL_IS_PANEL_CLIENT (panel), FALSE);

  priv = panel->priv;

  not_delayed = !priv->delayed_ready;

  if (not_delayed != delayed)
    return TRUE;

  if (priv->main_loop_running)
    {
      g_critical ("The delayed-ready property has to be set before starting "
                  "the main loop, but main loop is already running.");
      return FALSE;
    }

  priv->delayed_ready = delayed;

  return TRUE;
}

/**
 * mpl_panel_client_is_windowless:
 * @panel: #MplPanelClient
 *
 * Returns %TRUE if the associate panel is windowless (windowless panels have
 * a non-interactive button on the Toolbar).
 *
 * Return value: %TRUE if the panel does not have a panel associated with it.
 */
gboolean
mpl_panel_client_is_windowless (MplPanelClient *panel)
{
  MplPanelClientPrivate *priv;

  g_return_val_if_fail (MPL_IS_PANEL_CLIENT (panel), FALSE);

  priv = panel->priv;

  return priv->windowless;
}

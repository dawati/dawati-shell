/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/proxy-subclass.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/defs.h>
#include <telepathy-glib/svc-client.h>
#include <telepathy-glib/svc-generic.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/channel.h>

#include "anerley-tp-observer.h"
#include "anerley-marshals.h"

static void
client_observer_iface_init (gpointer g_iface,
                            gpointer g_iface_data);

G_DEFINE_TYPE_WITH_CODE (AnerleyTpObserver,
  anerley_tp_observer,
  G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
    tp_dbus_properties_mixin_iface_init);
  G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT, NULL);
  G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT_OBSERVER,
    client_observer_iface_init);
);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_TP_OBSERVER, AnerleyTpObserverPrivate))

typedef struct _AnerleyTpObserverPrivate AnerleyTpObserverPrivate;

struct _AnerleyTpObserverPrivate {
  GPtrArray *filters;
  gchar *bus_name;
  GHashTable *connections;
  TpDBusDaemon *bus;
};

enum
{
  PROP_0,
  PROP_CHANNEL_FILTER,
  PROP_INTERFACES,
};

enum
{
  NEW_CHANNEL_SIGNAL,
  LAST_SIGNAL
};

guint signals[LAST_SIGNAL];

static const gchar *anerley_tp_observer_interfaces[] = {
  TP_IFACE_CLIENT_OBSERVER,
  NULL
};

static void
anerley_tp_observer_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyTpObserverPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CHANNEL_FILTER:
      g_value_set_boxed (value, priv->filters);
      break;
    case PROP_INTERFACES:
      g_value_set_boxed (value, anerley_tp_observer_interfaces);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_observer_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_observer_dispose (GObject *object)
{
  AnerleyTpObserverPrivate *priv = GET_PRIVATE (object);

  if (priv->connections)
  {
    g_hash_table_unref (priv->connections);
    priv->connections = NULL;
  }

  if (priv->bus)
  {
    tp_dbus_daemon_release_name (priv->bus, priv->bus_name, NULL);
    g_object_unref (priv->bus);
    priv->bus = NULL;
  }

  G_OBJECT_CLASS (anerley_tp_observer_parent_class)->dispose (object);
}

static void
anerley_tp_observer_finalize (GObject *object)
{
  AnerleyTpObserverPrivate *priv = GET_PRIVATE (object);

  g_free (priv->bus_name);
  g_boxed_free (TP_ARRAY_TYPE_CHANNEL_CLASS_LIST, priv->filters);


  G_OBJECT_CLASS (anerley_tp_observer_parent_class)->finalize (object);
}

static void
anerley_tp_observer_class_init (AnerleyTpObserverClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  static TpDBusPropertiesMixinPropImpl client_props[] = {
    { "Interfaces", "interfaces", NULL },
    { NULL }
  };

  static TpDBusPropertiesMixinPropImpl client_observer_props[] = {
    { "ObserverChannelFilter", "channel-filter", NULL },
    { NULL }
  };

  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
    { TP_IFACE_CLIENT,
      tp_dbus_properties_mixin_getter_gobject_properties,
      NULL,
      client_props
    },
    { TP_IFACE_CLIENT_OBSERVER,
      tp_dbus_properties_mixin_getter_gobject_properties,
      NULL,
      client_observer_props
    },
    { NULL }
  };

  g_type_class_add_private (klass, sizeof (AnerleyTpObserverPrivate));

  object_class->get_property = anerley_tp_observer_get_property;
  object_class->set_property = anerley_tp_observer_set_property;
  object_class->dispose = anerley_tp_observer_dispose;
  object_class->finalize = anerley_tp_observer_finalize;

  klass->dbus_props_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
                                       G_STRUCT_OFFSET (AnerleyTpObserverClass,
                                                        dbus_props_class));


  pspec = g_param_spec_boxed ("interfaces",
                              "interfaces",
                              "Implemented D-Bus interfaces",
                              G_TYPE_STRV,
                              G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_INTERFACES, pspec);

  pspec = g_param_spec_boxed ("channel-filter",
                              "channel-filter",
                              "Filter for channels to be observed",
                              TP_ARRAY_TYPE_CHANNEL_CLASS_LIST,
                              G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_CHANNEL_FILTER, pspec);

  signals[NEW_CHANNEL_SIGNAL] = g_signal_new ("new-channel",
                                              ANERLEY_TYPE_TP_OBSERVER,
                                              G_SIGNAL_RUN_LAST,
                                              0,
                                              NULL,
                                              NULL,
                                              anerley_marshal_VOID__STRING_OBJECT,
                                              G_TYPE_NONE,
                                              2,
                                              G_TYPE_STRING,
                                              TP_TYPE_CHANNEL);
}

static void
anerley_tp_observer_init (AnerleyTpObserver *self)
{
  AnerleyTpObserverPrivate *priv = GET_PRIVATE (self);
  GHashTable *asv;
  gchar *object_path;

  priv->filters = g_ptr_array_new ();

  asv = tp_asv_new (
        TP_IFACE_CHANNEL ".ChannelType",
          G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
        TP_IFACE_CHANNEL ".TargetHandleType",
          G_TYPE_INT, TP_HANDLE_TYPE_CONTACT,
        NULL);
  g_ptr_array_add (priv->filters, asv);

  priv->bus = tp_dbus_daemon_dup (NULL);

  priv->bus_name = g_strdup_printf (TP_CLIENT_BUS_NAME_BASE "Anerley%d",
                                    (gint)getpid());
  object_path = g_strdup_printf (TP_CLIENT_OBJECT_PATH_BASE "Anerley%d",
                                 (gint)getpid());

  g_debug ("Ding ding on the bus: %s", priv->bus_name);
  tp_dbus_daemon_request_name (priv->bus,
                               priv->bus_name,
                               TRUE,
                               NULL);

  dbus_g_connection_register_g_object (tp_get_bus (),
                                       object_path,
                                       G_OBJECT (self));

  g_free (object_path);

  priv->connections = g_hash_table_new_full (g_str_hash,
                                             g_str_equal,
                                             g_free,
                                             g_object_unref);
}

AnerleyTpObserver *
anerley_tp_observer_new (void)
{
  return g_object_new (ANERLEY_TYPE_TP_OBSERVER, NULL);
}

typedef struct
{
  AnerleyTpObserver *observer;
  gchar *channel;
  gchar *account_name;
} ReadyClosure;

static void
_channel_ready_cb (TpChannel    *channel,
                   const GError *error_in,
                   gpointer      userdata)
{
  ReadyClosure *closure = (ReadyClosure *)userdata;

  if (error_in)
  {
    g_warning (G_STRLOC ": Error when making channel ready: %s",
               error_in->message);
    goto done;
  }

  g_signal_emit (closure->observer,
                 signals [NEW_CHANNEL_SIGNAL],
                 0,
                 closure->account_name,
                 channel);

done:
  g_object_unref (closure->observer);
  g_free (closure->channel);
  g_free (closure->account_name);
  g_free (closure);
}

static void 
_connection_ready_cb (TpConnection *connection,
                      const GError *error_in,
                      gpointer      userdata)
{
  ReadyClosure *closure = (ReadyClosure *)userdata;
  TpChannel *channel;
  GError *error = NULL;

  if (error_in)
  {
    g_warning (G_STRLOC ": Error when making connection ready: %s",
               error_in->message);
    goto error;
  }

  channel = tp_channel_new (connection,
                            closure->channel,
                            TP_IFACE_CHANNEL_TYPE_TEXT,
                            TP_HANDLE_TYPE_CONTACT,
                            0,
                            &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error creating channel: %s",
               error->message);
    g_clear_error (&error);
    goto error;
  } else {
    g_debug (G_STRLOC ": Created a channel for %s",
             closure->channel);
    tp_channel_call_when_ready (channel,
                                _channel_ready_cb,
                                closure);
  }

  return;

error:
  g_object_unref (closure->observer);
  g_free (closure->channel);
  g_free (closure->account_name);
  g_free (closure);
}

static void
_observer_observe_channels (TpSvcClientObserver   *observer,
                            const gchar           *account_obj_path,
                            const gchar           *connection_obj_path,
                            const GPtrArray       *channels,
                            const gchar           *dispatch_operation,
                            const GPtrArray       *requests_satisfied,
                            GHashTable            *observer_info,
                            DBusGMethodInvocation *context)
{
  AnerleyTpObserverPrivate *priv = GET_PRIVATE (observer);
  TpConnection *connection;
  gint i = 0;
  GValue *value;
  GValueArray *value_array;
  GError *error = NULL;
  const gchar *channel;
  ReadyClosure *closure;

  g_debug (G_STRLOC ": Observed: account = %s, connection = %s",
           account_obj_path,
           connection_obj_path);

  for (i = 0; i < channels->len; i++)
  {
    value_array = (GValueArray *)channels->pdata[i];
    value = g_value_array_get_nth (value_array, 0);

    /* It's a string but it's boxed. wtf... */
    channel = g_value_get_boxed (value);
    g_debug (G_STRLOC ": Got channel: %s",
             channel);
  }

  /* Check if we have this connection */
  connection = g_hash_table_lookup (priv->connections,
                                    connection_obj_path);

  if (!connection)
  {
    g_debug (G_STRLOC ": Creating new connection for %s",
             connection_obj_path);
    connection = tp_connection_new (priv->bus,
                                    NULL,
                                    connection_obj_path,
                                    &error);

    if (error)
    {
      g_warning (G_STRLOC ": Error creating connection: %s",
                 error->message);
      g_clear_error (&error);
    } else {
      g_hash_table_insert (priv->connections,
                           g_strdup (connection_obj_path),
                           connection);
    }
  }

  if (connection)
  {
    g_debug (G_STRLOC ": Got a connection for %s", connection_obj_path);

    for (i = 0; i < channels->len; i++)
    {
      value_array = (GValueArray *)channels->pdata[i];
      value = g_value_array_get_nth (value_array, 0);

      /* It's a string but it's boxed. wtf... */
      channel = g_value_get_boxed (value);
      g_debug (G_STRLOC ": Setting up 'ready' on connection for channel: %s",
               channel);

      closure = g_new0 (ReadyClosure, 1);
      closure->observer = g_object_ref (observer);
      closure->channel = g_strdup (channel);
      closure->account_name = g_strdup (account_obj_path);
      tp_connection_call_when_ready (connection,
                                     _connection_ready_cb,
                                     closure);

    }
  }

  tp_svc_client_observer_return_from_observe_channels (context);
}

static void
client_observer_iface_init (gpointer g_iface,
                            gpointer g_iface_data)
{
  TpSvcClientObserverClass *klass = (TpSvcClientObserverClass *) g_iface;

  tp_svc_client_observer_implement_observe_channels (klass,
                                                     _observer_observe_channels);
}


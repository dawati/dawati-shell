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

#include "anerley-tp-monitor-feed.h"

#include <anerley/anerley-feed.h>
#include <anerley/anerley-item.h>
#include <anerley/anerley-tp-feed.h>
#include <telepathy-glib/channel.h>
#include <folks/folks-telepathy.h>

G_DEFINE_TYPE_WITH_CODE (AnerleyTpMonitorFeed,
                         anerley_tp_monitor_feed,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (ANERLEY_TYPE_FEED,
                                                NULL));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_TP_MONITIOR_FEED, AnerleyTpMonitorFeedPrivate))

typedef struct _AnerleyTpMonitorFeedPrivate AnerleyTpMonitorFeedPrivate;

struct _AnerleyTpMonitorFeedPrivate {
  FolksIndividualAggregator *aggregator;
  gchar *client_name;

  TpBaseClient *observer;
  GHashTable *channels_to_items;
};

enum
{
  PROP_0,
  PROP_AGGREGGATOR,
  PROP_CLIENT_NAME
};

static void
anerley_tp_monitor_feed_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_AGGREGGATOR:
      g_value_set_object (value, priv->aggregator);
      break;
    case PROP_CLIENT_NAME:
      g_value_set_string (value, priv->client_name);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_monitor_feed_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_AGGREGGATOR:
      priv->aggregator = g_value_dup_object (value);
      break;
    case PROP_CLIENT_NAME:
      priv->client_name = g_value_dup_string (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_monitor_feed_dispose (GObject *object)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (object);

  g_clear_object (&priv->aggregator);
  g_clear_object (&priv->observer);
  tp_clear_pointer (&priv->channels_to_items, g_hash_table_unref);

  G_OBJECT_CLASS (anerley_tp_monitor_feed_parent_class)->dispose (object);
}

static void
anerley_tp_monitor_feed_finalize (GObject *object)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (object);

  g_free (priv->client_name);

  G_OBJECT_CLASS (anerley_tp_monitor_feed_parent_class)->finalize (object);
}

static void
_channel_invalidated_cb (TpChannel *channel,
    guint domain,
    gint code,
    gchar *message,
    AnerleyTpMonitorFeed *self)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (self);
  GList *items = NULL;
  AnerleyItem *item;

  g_debug (G_STRLOC ": Channel with identifier %s invalidated: %s",
           tp_channel_get_identifier (channel), message);

  item = g_hash_table_lookup (priv->channels_to_items, channel);

  items = g_list_append (NULL, item);
  g_signal_emit_by_name (self, "items-removed", items);
  g_list_free (items);

  g_hash_table_remove (priv->channels_to_items, channel);
}

/* Copied from empathy-utils.c, with copyright holder permission */
FolksIndividual *
empathy_create_individual_from_tp_contact (TpContact *contact)
{
  GeeSet *personas;
  TpfPersona *persona;
  FolksIndividual *individual;

  persona = tpf_persona_dup_for_contact (contact);
  if (persona == NULL)
    {
      g_debug ("Failed to get a persona for %s",
          tp_contact_get_identifier (contact));
      return NULL;
    }

  personas = GEE_SET (
      gee_hash_set_new (FOLKS_TYPE_PERSONA, g_object_ref, g_object_unref,
      g_direct_hash, g_direct_equal));

  gee_collection_add (GEE_COLLECTION (personas), persona);

  individual = folks_individual_new (personas);

  g_clear_object (&persona);
  g_clear_object (&personas);

  return individual;
}

static void
_observer_new_channel_cb (TpSimpleObserver           *observer,
                          TpAccount                  *account,
                          TpConnection               *connection,
                          GList                      *channels,
                          TpChannelDispatchOperation *dispatch_operation,
                          GList                      *requests,
                          TpObserveChannelsContext   *context,
                          gpointer                    user_data)
{
  AnerleyTpMonitorFeed *self = ANERLEY_TP_MONITIOR_FEED (user_data);
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (user_data);
  TpChannel *channel;
  AnerleyItem *item;
  GList *added_items;
  TpContact *target;
  FolksIndividual *contact;

  if (channels == NULL || channels->next != NULL)
  {
    GError e = { TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
                 "Expecting one and only one channel" };
    tp_observe_channels_context_fail (context, &e);
    return;
  }

  channel = channels->data;

  if (!TP_IS_TEXT_CHANNEL (channel))
  {
    GError e = { TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
                "Expecting a TpTextChannel" };
    tp_observe_channels_context_fail (context, &e);
    return;
  }

  if (g_hash_table_lookup (priv->channels_to_items, channel) != NULL)
  {
    GError e = { TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
                 "We are already observing this channel" };
    tp_observe_channels_context_fail (context, &e);
    return;
  }

  /* FIXME: Should take the individual from the aggregator */
  target = tp_channel_get_target_contact (channel);
  contact = empathy_create_individual_from_tp_contact (target);
  item = anerley_item_new (contact);
  g_hash_table_insert (priv->channels_to_items, g_object_ref (channel), item);
  g_object_unref (contact);

  anerley_item_associate_channel (ANERLEY_ITEM (item),
                                  TP_TEXT_CHANNEL (channel));

  g_signal_connect (channel, "invalidated",
                    G_CALLBACK (_channel_invalidated_cb),
                    self);

  added_items = g_list_append (NULL, item);
  g_signal_emit_by_name (self, "items-added", added_items);
  g_list_free (added_items);

  tp_observe_channels_context_accept (context);
}

static void
anerley_tp_monitor_feed_constructed (GObject *object)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (object);
  TpAccountManager *am;

  /* Observe private text channels */
  am = tp_account_manager_dup ();
  priv->observer = tp_simple_observer_new_with_am (am,
                                                   TRUE,
                                                   priv->client_name,
                                                   TRUE,
                                                   _observer_new_channel_cb,
                                                   object,
                                                   NULL);
  tp_base_client_take_observer_filter (priv->observer, tp_asv_new (
      TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_INT, TP_HANDLE_TYPE_CONTACT,
      NULL));
  g_object_unref (am);

  tp_base_client_register (priv->observer, NULL);

  if (G_OBJECT_CLASS (anerley_tp_monitor_feed_parent_class)->constructed)
    G_OBJECT_CLASS (anerley_tp_monitor_feed_parent_class)->constructed (object);
}

static void
anerley_tp_monitor_feed_class_init (AnerleyTpMonitorFeedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyTpMonitorFeedPrivate));

  object_class->get_property = anerley_tp_monitor_feed_get_property;
  object_class->set_property = anerley_tp_monitor_feed_set_property;
  object_class->dispose = anerley_tp_monitor_feed_dispose;
  object_class->finalize = anerley_tp_monitor_feed_finalize;
  object_class->constructed = anerley_tp_monitor_feed_constructed;

  pspec = g_param_spec_string ("client-name",
                               "Client name",
                               "Name for the Telepathy client",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_CLIENT_NAME, pspec);

  pspec = g_param_spec_object ("aggregator",
                               "Folks Aggregator",
                               "The Folks individual aggregator",
                               FOLKS_TYPE_INDIVIDUAL_AGGREGATOR,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_AGGREGGATOR, pspec);
}

static void
anerley_tp_monitor_feed_init (AnerleyTpMonitorFeed *self)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (self);

  priv->channels_to_items = g_hash_table_new_full (g_direct_hash,
                                                   g_direct_equal,
                                                   g_object_unref,
                                                   g_object_unref);
}

AnerleyFeed *
anerley_tp_monitor_feed_new (FolksIndividualAggregator *aggregator,
    const gchar *client_name)
{
  return g_object_new (ANERLEY_TYPE_TP_MONITIOR_FEED,
                       "aggregator", aggregator,
                       "client-name", client_name,
                       NULL);
}

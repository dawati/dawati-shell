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

#include <telepathy-glib/telepathy-glib.h>

#include "anerley-feed.h"
#include "anerley-item.h"
#include "anerley-tp-feed.h"

static void feed_interface_init (gpointer g_iface, gpointer iface_data);
G_DEFINE_TYPE_WITH_CODE (AnerleyTpFeed,
                         anerley_tp_feed,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (ANERLEY_TYPE_FEED,
                                                feed_interface_init));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_TP_FEED, AnerleyTpFeedPrivate))

typedef struct _AnerleyTpFeedPrivate AnerleyTpFeedPrivate;

struct _AnerleyTpFeedPrivate {
  FolksIndividualAggregator *aggregator;
  GHashTable *ids_to_items;
};

enum
{
  PROP_0,
  PROP_AGGREGGATOR
};

static void
anerley_tp_feed_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_AGGREGGATOR:
      g_value_set_object (value, priv->aggregator);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_feed_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_AGGREGGATOR:
      priv->aggregator = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_feed_dispose (GObject *object)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (object);

  g_clear_object (&priv->aggregator);
  tp_clear_pointer (&priv->ids_to_items, g_hash_table_unref);

  G_OBJECT_CLASS (anerley_tp_feed_parent_class)->dispose (object);
}

static AnerleyItem *
_make_item_from_contact (AnerleyTpFeed *feed,
                         FolksIndividual *contact)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  AnerleyItem *item;

  item = anerley_item_new (contact);

  /* Move the ownership of the reference to the hash table */
  g_hash_table_insert (priv->ids_to_items,
                       g_strdup (folks_individual_get_id (contact)),
                       item);

  return item;
}

/* Modified code from empathy, with copyright holder permission */
static void
aggregator_individuals_changed_cb (FolksIndividualAggregator *aggregator,
                                   GeeMultiMap               *changes,
                                   AnerleyTpFeed             *self)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (self);
  GeeIterator *iter;
  GeeSet *removed;
  GeeCollection *added;
  GList *added_set = NULL, *removed_set = NULL;

  /* We're not interested in the relationships between the added and removed
   * individuals, so just extract collections of them. Note that the added
   * collection may contain duplicates, while the removed set won't. */
  removed = gee_multi_map_get_keys (changes);
  added = gee_multi_map_get_values (changes);

  /* Handle the removals first, as one of the added Individuals might have the
   * same ID as one of the removed Individuals (due to linking). */
  iter = gee_iterable_iterator (GEE_ITERABLE (removed));
  while (gee_iterator_next (iter))
  {
    FolksIndividual *ind = gee_iterator_get (iter);
    AnerleyItem *item;

    if (ind == NULL)
      continue;

    item = g_hash_table_lookup (priv->ids_to_items,
                               folks_individual_get_id (ind));
    if (item == NULL)
    {
      g_object_unref (ind);
      continue;
    }

    removed_set = g_list_prepend (removed_set, g_object_ref (item));
    g_hash_table_remove (priv->ids_to_items,
                         folks_individual_get_id (ind));

    g_clear_object (&ind);
  }
  g_clear_object (&iter);

  iter = gee_iterable_iterator (GEE_ITERABLE (added));
  while (gee_iterator_next (iter))
  {
    FolksIndividual *ind = gee_iterator_get (iter);
    AnerleyItem *item;

    if (ind == NULL)
      continue;

    item = g_hash_table_lookup (priv->ids_to_items,
        folks_individual_get_id (ind));
    if (item != NULL)
    {
      g_object_unref (ind);
      continue;
    }

    item = _make_item_from_contact (self, ind);
    added_set = g_list_prepend (added_set, g_object_ref (item));

    g_clear_object (&ind);
  }
  g_clear_object (&iter);

  if (removed_set != NULL)
  {
    g_signal_emit_by_name (self, "items-removed", removed_set);
    g_list_free_full (removed_set, g_object_unref);
  }

  if (added_set != NULL)
  {
    g_signal_emit_by_name (self, "items-added", added_set);
    g_list_free_full (added_set, g_object_unref);
  }

  g_object_unref (added);
  g_object_unref (removed);
}

static void
anerley_tp_feed_constructed (GObject *object)
{
  AnerleyTpFeed *self = ANERLEY_TP_FEED (object);
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (self);

  G_OBJECT_CLASS (anerley_tp_feed_parent_class)->constructed (object);

  g_signal_connect (priv->aggregator, "individuals-changed-detailed",
                    G_CALLBACK (aggregator_individuals_changed_cb),
                    self);
  folks_individual_aggregator_prepare (priv->aggregator, NULL, NULL);
}

static void
anerley_tp_feed_class_init (AnerleyTpFeedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyTpFeedPrivate));

  object_class->constructed = anerley_tp_feed_constructed;
  object_class->get_property = anerley_tp_feed_get_property;
  object_class->set_property = anerley_tp_feed_set_property;
  object_class->dispose = anerley_tp_feed_dispose;

  pspec = g_param_spec_object ("aggregator",
                               "Folks Aggregator",
                               "The Folks individual aggregator",
                               FOLKS_TYPE_INDIVIDUAL_AGGREGATOR,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_AGGREGGATOR, pspec);
}

static void
anerley_tp_feed_init (AnerleyTpFeed *self)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (self);

  priv->ids_to_items = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              (GDestroyNotify)g_object_unref);
}

AnerleyTpFeed *
anerley_tp_feed_new (FolksIndividualAggregator *aggregator)
{
  return g_object_new (ANERLEY_TYPE_TP_FEED,
                       "aggregator", aggregator,
                       NULL);
}

static void
feed_interface_init (gpointer g_iface,
                     gpointer iface_data)
{
  /* Nothing to do here..? */
}

AnerleyItem *
anerley_tp_feed_get_item_by_uid (AnerleyTpFeed *feed,
                                 const gchar   *uid)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);

  return g_hash_table_lookup (priv->ids_to_items, uid);
}

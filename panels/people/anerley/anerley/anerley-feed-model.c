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


#include "anerley-feed-model.h"

#include <anerley/anerley-item.h>
#include <telepathy-glib/telepathy-glib.h>

#include <string.h>

G_DEFINE_TYPE (AnerleyFeedModel, anerley_feed_model, CLUTTER_TYPE_LIST_MODEL)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_FEED_MODEL, AnerleyFeedModelPrivate))

typedef struct _AnerleyFeedModelPrivate AnerleyFeedModelPrivate;

struct _AnerleyFeedModelPrivate {
  AnerleyFeed *feed;
  gchar *filter_text;
  gboolean show_offline;
  AnerleyFeedModelSortMethod sort_method;
};

enum
{
  PROP_0,
  PROP_FEED,
  PROP_FILTER_TEXT
};

enum
{
  BULK_CHANGE_START,
  BULK_CHANGE_END,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void anerley_feed_model_update_feed (AnerleyFeedModel *model,
                                            AnerleyFeed      *feed);

static void
anerley_feed_model_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyFeedModelPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_FEED:
      g_value_set_object (value, priv->feed);
      break;
    case PROP_FILTER_TEXT:
      g_value_set_string (value, priv->filter_text);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_feed_model_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AnerleyFeedModel *model = (AnerleyFeedModel *)object;

  switch (property_id) {
    case PROP_FEED:
      anerley_feed_model_update_feed (model, g_value_get_object (value));
      break;
    case PROP_FILTER_TEXT:
      anerley_feed_model_set_filter_text (model, g_value_get_string (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_feed_model_dispose (GObject *object)
{
  AnerleyFeedModelPrivate *priv = GET_PRIVATE (object);

  if (priv->feed)
  {
    g_object_unref (priv->feed);
    priv->feed = NULL;
  }

  G_OBJECT_CLASS (anerley_feed_model_parent_class)->dispose (object);
}

static void
anerley_feed_model_finalize (GObject *object)
{
  AnerleyFeedModelPrivate *priv = GET_PRIVATE (object);

  if (priv->filter_text)
  {
    g_free (priv->filter_text);
  }

  G_OBJECT_CLASS (anerley_feed_model_parent_class)->finalize (object);
}

static void
anerley_feed_model_class_init (AnerleyFeedModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyFeedModelPrivate));

  object_class->get_property = anerley_feed_model_get_property;
  object_class->set_property = anerley_feed_model_set_property;
  object_class->dispose = anerley_feed_model_dispose;
  object_class->finalize = anerley_feed_model_finalize;

  pspec = g_param_spec_object ("feed",
                               "The feed",
                               "The feed to show.",
                               ANERLEY_TYPE_FEED,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FEED, pspec);

  pspec = g_param_spec_string ("filter-text",
                               "Filtering text",
                               "The string on which items should be filtered",
                               NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FILTER_TEXT, pspec);

  signals[BULK_CHANGE_START] =
    g_signal_new ("bulk-change-start",
                  ANERLEY_TYPE_FEED_MODEL,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  signals[BULK_CHANGE_END] =
    g_signal_new ("bulk-change-end",
                  ANERLEY_TYPE_FEED_MODEL,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}

static gint
_model_sort_func_name (ClutterModel *model,
                       const GValue *a,
                       const GValue *b,
                       gpointer      userdata)
{
  AnerleyItem *item_a;
  AnerleyItem *item_b;
  const gchar *str_a;
  const gchar *str_b;

  item_a = g_value_get_object (a);
  item_b = g_value_get_object (b);

  /* Already g_utf8_casefold'ed */
  str_a = anerley_item_get_sortable_name (item_a);
  str_b = anerley_item_get_sortable_name (item_b);

  return g_utf8_collate (str_a, str_b);
}

static gint
_model_sort_func_presence (ClutterModel *model,
                           const GValue *a,
                           const GValue *b,
                           gpointer      userdata)
{
  AnerleyItem *item_a;
  AnerleyItem *item_b;
  TpConnectionPresenceType presence_a;
  TpConnectionPresenceType presence_b;
  gint ret_val;

  item_a = g_value_get_object (a);
  item_b = g_value_get_object (b);

  /* TpConnectionPresenceType as exact same values than FolksPresenceType.... */
  presence_a = (TpConnectionPresenceType) anerley_item_get_presence_type (item_a);
  presence_b = (TpConnectionPresenceType) anerley_item_get_presence_type (item_b);

  ret_val = -tp_connection_presence_type_cmp_availability (presence_a, presence_b);
  if (ret_val == 0)
    {
      /* Fallback: compare by name */
      ret_val = _model_sort_func_name (model, a, b, userdata);
    }

  return ret_val;
}

static gint
_model_sort_func (ClutterModel *model,
                  const GValue *a,
                  const GValue *b,
                  gpointer      userdata)
{
  AnerleyFeedModelPrivate *priv = GET_PRIVATE (model);

  switch (priv->sort_method)
  {
    case ANERLEY_FEED_MODEL_SORT_METHOD_NAME:
      return _model_sort_func_name (model, a, b, userdata);
    case ANERLEY_FEED_MODEL_SORT_METHOD_PRESENCE:
      return _model_sort_func_presence (model, a, b, userdata);
  }

  return 0;
}

static gboolean
_model_filter_cb (ClutterModel     *model,
                  ClutterModelIter *iter,
                  gpointer          userdata)
{
  AnerleyFeedModelPrivate *priv = GET_PRIVATE (model);
  AnerleyItem *item = NULL;
  gboolean ret = FALSE;

  clutter_model_iter_get (iter, 0, &item, -1);

  if (G_UNLIKELY (item == NULL))
    return FALSE;

  if (!anerley_item_is_im (item))
    goto out;

  if (!priv->show_offline && !anerley_item_is_online (item))
    goto out;

  if (priv->filter_text != NULL &&
      strcasestr (anerley_item_get_display_name (item),
                  priv->filter_text) == NULL)
    goto out;

  ret = TRUE;

out:
  g_object_unref (item);

  return ret;
}

static void
anerley_feed_model_init (AnerleyFeedModel *self)
{
  GType types[] = { ANERLEY_TYPE_ITEM };

  clutter_model_set_types (CLUTTER_MODEL (self),
                           1,
                           types);
  clutter_model_set_filter (CLUTTER_MODEL (self),
                            _model_filter_cb,
                            NULL,
                            NULL);
}

ClutterModel *
anerley_feed_model_new (AnerleyFeed *feed)
{
  return g_object_new (ANERLEY_TYPE_FEED_MODEL,
                       "feed",
                       feed,
                       NULL);
}

static void
refilter_all (AnerleyFeedModel *model)
{
  g_signal_emit_by_name (model, "filter-changed");
}

static void
refilter_item (AnerleyFeedModel *model,
               AnerleyItem      *item)
{
  /* We can't find unvisible rows, so its not possible to simply emit
   * row-changed for that item... so let's just refilter everything...
   * Thanks clutter, thanks! */
  refilter_all (model);
}

static void
_feed_items_added_cb (AnerleyFeed *feed,
                      GList       *items,
                      gpointer     userdata)
{
  AnerleyFeedModel *model = (AnerleyFeedModel *)userdata;
  GList *l;
  AnerleyItem *item;

  g_signal_emit (model, signals[BULK_CHANGE_START], 0);

  clutter_model_set_sort ((ClutterModel *)model,
                          -1,
                          NULL,
                          NULL,
                          NULL);


  for (l = items; l; l = l->next)
  {
    item = (AnerleyItem *)l->data;

    tp_g_signal_connect_object (item, "presence-changed",
                                G_CALLBACK (refilter_item),
                                model, G_CONNECT_SWAPPED);
    tp_g_signal_connect_object (item, "display-name-changed",
                                G_CALLBACK (refilter_item),
                                model, G_CONNECT_SWAPPED);

    clutter_model_append (CLUTTER_MODEL (model),
                          0,
                          item,
                          -1);
  }

  clutter_model_set_sort ((ClutterModel *)model,
                          0,
                          _model_sort_func,
                          NULL,
                          NULL);

  g_signal_emit (model, signals[BULK_CHANGE_END], 0);
}

static void
_feed_items_removed_cb (AnerleyFeed *feed,
                        GList       *items,
                        gpointer     userdata)
{
  AnerleyFeedModel *model = (AnerleyFeedModel *)userdata;
  GList *l;
  AnerleyItem *item_to_remove, *item;
  ClutterModelIter *iter;

  g_signal_emit (model, signals[BULK_CHANGE_START], 0);

  /* If we have a filter set we must remove it before we can iterate */

  if (clutter_model_get_filter_set ((ClutterModel *)model))
  {
    clutter_model_set_filter ((ClutterModel *)model,
                              NULL,
                              NULL,
                              NULL);
  }

  for (l = items; l; l = l->next)
  {
    item_to_remove = (AnerleyItem *)l->data;

    iter = clutter_model_get_first_iter ((ClutterModel *)model);
    if (iter == NULL)
      break;

    while (!clutter_model_iter_is_last (iter))
    {
      clutter_model_iter_get (iter,
                              0,
                              &item,
                              -1);

      if (item == item_to_remove)
      {
        g_signal_handlers_disconnect_by_func (item, refilter_item, feed);
        clutter_model_remove ((ClutterModel *)model,
                              clutter_model_iter_get_row (iter));
        break;
      }

      clutter_model_iter_next (iter);
    }

    g_object_unref (iter);
  }

  clutter_model_set_filter ((ClutterModel *)model,
                            _model_filter_cb,
                            NULL,
                            NULL);

  g_signal_emit (model, signals[BULK_CHANGE_END], 0);
}

static void
anerley_feed_model_update_feed (AnerleyFeedModel *model,
                                AnerleyFeed      *feed)
{
  AnerleyFeedModelPrivate *priv = GET_PRIVATE (model);

  if (priv->feed)
  {
    g_signal_handlers_disconnect_by_func (priv->feed,
                                          _feed_items_added_cb,
                                          model);
    g_signal_handlers_disconnect_by_func (priv->feed,
                                          _feed_items_removed_cb,
                                          model);
    g_object_unref (priv->feed);
    priv->feed = NULL;
  }

  if (feed)
  {
    priv->feed = g_object_ref (feed);
    g_signal_connect (priv->feed,
                      "items-added",
                      (GCallback)_feed_items_added_cb,
                      model);
    g_signal_connect (priv->feed,
                      "items-removed",
                      (GCallback)_feed_items_removed_cb,
                      model);
  }
}

void
anerley_feed_model_set_filter_text (AnerleyFeedModel *model,
                                    const gchar      *filter_text)
{
  AnerleyFeedModelPrivate *priv = GET_PRIVATE (model);

  if (!tp_strdiff (filter_text, priv->filter_text))
    return;

  g_free (priv->filter_text);
  priv->filter_text = g_strdup (filter_text);

  refilter_all (model);
}

void
anerley_feed_model_set_show_offline (AnerleyFeedModel *model,
                                     gboolean          show_offline)
{
  AnerleyFeedModelPrivate *priv = GET_PRIVATE (model);

  if (priv->show_offline == show_offline)
    return;

  priv->show_offline = show_offline;

  refilter_all (model);
}

void
anerley_feed_model_set_sort_method (AnerleyFeedModel *model,
                                    AnerleyFeedModelSortMethod method)
{
  AnerleyFeedModelPrivate *priv = GET_PRIVATE (model);

  if (method == priv->sort_method)
    return;

  priv->sort_method = method;

  clutter_model_resort (CLUTTER_MODEL (model));
}

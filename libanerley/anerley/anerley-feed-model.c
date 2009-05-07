#include "anerley-feed-model.h"

#include <anerley/anerley-item.h>

#include <string.h>

G_DEFINE_TYPE (AnerleyFeedModel, anerley_feed_model, CLUTTER_TYPE_LIST_MODEL)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_FEED_MODEL, AnerleyFeedModelPrivate))

typedef struct _AnerleyFeedModelPrivate AnerleyFeedModelPrivate;

struct _AnerleyFeedModelPrivate {
  AnerleyFeed *feed;
  gchar *filter_text;
};

enum
{
  PROP_0,
  PROP_FEED,
  PROP_FILTER_TEXT
};

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
}

static gint
_model_sort_func (ClutterModel *model,
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

static void
anerley_feed_model_init (AnerleyFeedModel *self)
{
  GType types[] = { ANERLEY_TYPE_ITEM };

  clutter_model_set_types (CLUTTER_MODEL (self),
                           1,
                           types);
  clutter_model_set_sort ((ClutterModel *)self,
                          0,
                          _model_sort_func,
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
_feed_items_added_cb (AnerleyFeed *feed,
                      GList       *items,
                      gpointer     userdata)
{
  AnerleyFeedModel *model = (AnerleyFeedModel *)userdata;
  GList *l;
  AnerleyItem *item;

  for (l = items; l; l = l->next)
  {
    item = (AnerleyItem *)l->data;

    clutter_model_append (CLUTTER_MODEL (model),
                          0,
                          item,
                          -1);
  }
}

static gboolean
_model_filter_cb (ClutterModel     *model,
                  ClutterModelIter *iter,
                  gpointer          userdata)
{
  const gchar *filter_text = (const gchar *)userdata;
  AnerleyItem *item = NULL;

  clutter_model_iter_get (iter, 0, &item, -1);

  if (G_LIKELY (item))
  {
    if (strcasestr (anerley_item_get_display_name (item),
                    filter_text))
    {
      g_object_unref (item);
      return TRUE;
    } else {
      g_object_unref (item);
      return FALSE;;
    }
  }

  return FALSE;
}

static void
_feed_items_removed_cb (AnerleyFeed *feed,
                        GList       *items,
                        gpointer     userdata)
{
  AnerleyFeedModel *model = (AnerleyFeedModel *)userdata;
  AnerleyFeedModelPrivate *priv = GET_PRIVATE (model);
  GList *l;
  AnerleyItem *item_to_remove, *item;
  ClutterModelIter *iter;

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
    while (!clutter_model_iter_is_last (iter))
    {
      clutter_model_iter_get (iter,
                              0,
                              &item,
                              -1);

      if (item == item_to_remove)
      {
        clutter_model_remove ((ClutterModel *)model,
                              clutter_model_iter_get_row (iter));
        break;
      }

      clutter_model_iter_next (iter);
    }

    g_object_unref (iter);
  }

  if (priv->filter_text)
  {
    clutter_model_set_filter ((ClutterModel *)model,
                              _model_filter_cb,
                              priv->filter_text,
                              NULL);
  }
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
  gchar *old_filter_text = NULL;

  if (priv->filter_text)
  {
    old_filter_text = priv->filter_text;
    priv->filter_text = NULL;
  }

  priv->filter_text = g_strdup (filter_text);

  if (priv->filter_text)
  {
    clutter_model_set_filter ((ClutterModel *)model,
                              _model_filter_cb,
                              priv->filter_text,
                              NULL);
  } else {
    clutter_model_set_filter ((ClutterModel *)model,
                              NULL,
                              NULL,
                              NULL);
  }

  g_free (old_filter_text);
}



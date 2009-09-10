/*
 *
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

#include "penge-magic-list-view.h"

G_DEFINE_TYPE (PengeMagicListView, penge_magic_list_view, PENGE_TYPE_MAGIC_CONTAINER)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_MAGIC_LIST_VIEW, PengeMagicListViewPrivate))

typedef struct _PengeMagicListViewPrivate PengeMagicListViewPrivate;

struct _PengeMagicListViewPrivate {
  ClutterModel *model;
  GList *attributes;
  GType item_type;
  gint freeze_count;
};

enum
{
  PROP_0,
  PROP_MODEL,
  PROP_ITEM_TYPE
};

typedef struct
{
  gchar *property_name;
  gint col;
} AttributeData;

static void
penge_magic_list_view_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeMagicListViewPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, priv->item_type);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_magic_list_view_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeMagicListView *view = PENGE_MAGIC_LIST_VIEW (object);

  switch (property_id) {
    case PROP_MODEL:
      penge_magic_list_view_set_model (view, g_value_get_object (value));
      break;
    case PROP_ITEM_TYPE:
      penge_magic_list_view_set_item_type (view, g_value_get_gtype (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_magic_list_view_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_magic_list_view_parent_class)->dispose (object);
}

static void
penge_magic_list_view_finalize (GObject *object)
{
  PengeMagicListViewPrivate *priv = GET_PRIVATE (object);
  GList *l;
  AttributeData *attr;

  for (l = priv->attributes; l; l = g_list_delete_link (l, l))
  {
    attr = (AttributeData *)l->data;
    g_free (attr->property_name);
    g_free (attr);
  }

  G_OBJECT_CLASS (penge_magic_list_view_parent_class)->finalize (object);
}

static void
penge_magic_list_view_class_init (PengeMagicListViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeMagicListViewPrivate));

  object_class->get_property = penge_magic_list_view_get_property;
  object_class->set_property = penge_magic_list_view_set_property;
  object_class->dispose = penge_magic_list_view_dispose;
  object_class->finalize = penge_magic_list_view_finalize;

  pspec = g_param_spec_object ("model",
                               "Model",
                               "Model to render",
                               CLUTTER_TYPE_MODEL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_MODEL, pspec);

  pspec = g_param_spec_gtype ("item-type",
                              "Item type",
                              "Item type to render",
                              CLUTTER_TYPE_ACTOR,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ITEM_TYPE, pspec);
}

static void
penge_magic_list_view_init (PengeMagicListView *self)
{
}

ClutterActor *
penge_magic_list_view_new (void)
{
  return g_object_new (PENGE_TYPE_MAGIC_LIST_VIEW, NULL);
}

static void
penge_magic_list_view_real_update (PengeMagicListView *view)
{
  PengeMagicListViewPrivate *priv = GET_PRIVATE (view);
  GList *children, *l, *ll = NULL;
  ClutterModelIter *iter = NULL;
  gint children_count = 0, model_count = 0;
  ClutterActor *tile;
  GValue value = { 0, };
  AttributeData *attr;
  gint delay = 0;
  ClutterAnimation *animation;
  ClutterTimeline *timeline;

  children = clutter_container_get_children (CLUTTER_CONTAINER (view));
  children_count = g_list_length (children);

  iter = clutter_model_get_first_iter (priv->model);

  while (iter && !clutter_model_iter_is_last (iter))
  {
    model_count++;
    clutter_model_iter_next (iter);
  }

  while (children_count < model_count)
  {
    tile = g_object_new (priv->item_type, NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (view),
                                 tile);
    children_count++;
  }

  g_list_free (children);

  children = clutter_container_get_children (CLUTTER_CONTAINER (view));

  if (iter)
    g_object_unref (iter);

  iter = clutter_model_get_first_iter (priv->model);
  for (l = children;
       l && iter && !clutter_model_iter_is_last (iter);
       l = l->next, clutter_model_iter_next (iter))
  {
    tile = (ClutterActor *)l->data;

    g_object_freeze_notify (G_OBJECT (tile));
    for (ll = priv->attributes; ll; ll = ll->next)
    {
      attr = (AttributeData *)ll->data;

      clutter_model_iter_get_value (iter, attr->col, &value);

      g_object_set_property (G_OBJECT (tile), attr->property_name, &value);
      g_value_unset (&value);
    }

    g_object_thaw_notify (G_OBJECT (tile));
  }

  for (; l; l = l->next)
  {
    clutter_container_remove_actor (CLUTTER_CONTAINER (view),
                                    (ClutterActor *)l->data);
  }

  g_list_free (children);

  children = clutter_container_get_children (CLUTTER_CONTAINER (view));

  for (l = children; l; l = l->next)
  {
    tile = (ClutterActor *)l->data;
    clutter_actor_set_opacity (tile, 0);
    animation = clutter_actor_animate (tile,
                                       CLUTTER_LINEAR,
                                       300,
                                       "opacity", 255,
                                       NULL);

    if (delay != 0)
    {
      timeline = clutter_animation_get_timeline (animation);
      clutter_timeline_stop (timeline);
      clutter_timeline_set_delay (timeline,
                                  delay);
      clutter_timeline_start (timeline);
    }

    delay += 150;
  }

  g_list_free (children);

  if (iter)
    g_object_unref (iter);
}

static void
_fade_animation_completed_cb (ClutterAnimation *animation,
                              gpointer          userdata)
{
  clutter_actor_set_opacity (CLUTTER_ACTOR (userdata), 255);
  penge_magic_list_view_real_update (PENGE_MAGIC_LIST_VIEW (userdata));
}

static void
penge_magic_list_view_update (PengeMagicListView *view)
{
  PengeMagicListViewPrivate *priv = GET_PRIVATE (view);
  ClutterAnimation *animation = NULL;

  if (!priv->item_type || !priv->model)
    return;

  g_debug (G_STRLOC ": Populating container from model using model: %s",
           G_OBJECT_TYPE_NAME (priv->model));


  animation = clutter_actor_animate (CLUTTER_ACTOR (view),
                                     CLUTTER_LINEAR,
                                     300,
                                     "opacity", 0,
                                     NULL);

  g_signal_connect_after (animation,
                          "completed",
                          (GCallback)_fade_animation_completed_cb,
                          view);
}

static void
_model_row_added_cb (ClutterModel     *model,
                     ClutterModelIter *iter,
                     gpointer          userdata)
{
  PengeMagicListView *view = PENGE_MAGIC_LIST_VIEW (userdata);

  penge_magic_list_view_real_update (view);
}

static void
_model_row_removed_cb (ClutterModel     *model,
                       ClutterModelIter *iter,
                       gpointer          userdata)
{
  PengeMagicListView *view = PENGE_MAGIC_LIST_VIEW (userdata);

  penge_magic_list_view_update (view);
}

static void
_model_row_changed_cb (ClutterModel     *model,
                       ClutterModelIter *iter,
                       gpointer          userdata)
{
  PengeMagicListView *view = PENGE_MAGIC_LIST_VIEW (userdata);

  penge_magic_list_view_update (view);
}

static void

_model_sort_changed_cb (ClutterModel *model,
                        gpointer      userdata)
{
  PengeMagicListView *view = PENGE_MAGIC_LIST_VIEW (userdata);

  penge_magic_list_view_update (view);
}

void
penge_magic_list_view_set_model (PengeMagicListView *view,
                                 ClutterModel       *model)
{
  PengeMagicListViewPrivate *priv = GET_PRIVATE (view);

  if (model != priv->model)
  {
    if (priv->model)
    {
      g_signal_handlers_disconnect_by_func (priv->model,
                                            (GCallback)_model_row_added_cb,
                                            view);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            (GCallback)_model_row_removed_cb,
                                            view);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            (GCallback)_model_row_changed_cb,
                                            view);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            (GCallback)_model_sort_changed_cb,
                                            view);

      g_object_unref (priv->model);
      priv->model = NULL;
    }

    priv->model = model;

    if (priv->model)
    {
      g_object_ref (priv->model);

      g_signal_connect (priv->model,
                        "row-added",
                        (GCallback)_model_row_added_cb,
                        view);
      g_signal_connect_after (priv->model,
                              "row-removed",
                              (GCallback)_model_row_removed_cb,
                              view);
      g_signal_connect (priv->model,
                        "row-changed",
                        (GCallback)_model_row_changed_cb,
                        view);
      g_signal_connect (priv->model,
                        "sort-changed",
                        (GCallback)_model_sort_changed_cb,
                        view);
    }
    penge_magic_list_view_update (view);
  }
}

void
penge_magic_list_view_set_item_type (PengeMagicListView *view,
                                     GType               item_type)
{
  PengeMagicListViewPrivate *priv = GET_PRIVATE (view);

  if (item_type != priv->item_type)
  {
    priv->item_type = item_type;
    penge_magic_list_view_update (view);
  }
}

void
penge_magic_list_view_add_attribute (PengeMagicListView *view,
                                     const gchar        *property_name,
                                     gint                col)
{
  PengeMagicListViewPrivate *priv = GET_PRIVATE (view);
  AttributeData *attr;

  attr = g_new0 (AttributeData, 1);
  attr->property_name = g_strdup (property_name);
  attr->col = col;

  priv->attributes = g_list_append (priv->attributes, attr);
}

void
penge_magic_list_view_freeze (PengeMagicListView *view)
{
  PengeMagicListViewPrivate *priv = GET_PRIVATE (view);

  priv->freeze_count++;

  if (priv->freeze_count > 0)
  {
    g_signal_handlers_block_by_func (priv->model,
                                     (GCallback)_model_row_added_cb,
                                     view);
    g_signal_handlers_block_by_func (priv->model,
                                     (GCallback)_model_row_removed_cb,
                                     view);
    g_signal_handlers_block_by_func (priv->model,
                                     (GCallback)_model_row_changed_cb,
                                     view);
    g_signal_handlers_block_by_func (priv->model,
                                     (GCallback)_model_sort_changed_cb,
                                     view);
  }
}

void
penge_magic_list_view_thaw (PengeMagicListView *view)
{
  PengeMagicListViewPrivate *priv = GET_PRIVATE (view);

  priv->freeze_count--;

  if (priv->freeze_count < 0)
  {
    g_assert_not_reached ();
  }

  if (priv->freeze_count == 0)
  {
    g_signal_handlers_unblock_by_func (priv->model,
                                       (GCallback)_model_row_added_cb,
                                       view);
    g_signal_handlers_unblock_by_func (priv->model,
                                       (GCallback)_model_row_removed_cb,
                                       view);
    g_signal_handlers_unblock_by_func (priv->model,
                                       (GCallback)_model_row_changed_cb,
                                       view);
    g_signal_handlers_unblock_by_func (priv->model,
                                       (GCallback)_model_sort_changed_cb,
                                       view);

    penge_magic_list_view_update (view);
  }
}



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
  GType item_type;
};

enum
{
  PROP_0,
  PROP_MODEL,
  PROP_ITEM_TYPE
};

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
penge_magic_list_view_update (PengeMagicListView *view)
{
  PengeMagicListViewPrivate *priv = GET_PRIVATE (view);
  GList *children, *l = NULL;
  ClutterModelIter *iter = NULL;
  gint children_count = 0, model_count = 0;
  ClutterActor *tile;
  GValue value = { 0, };


  if (!priv->item_type || !priv->model)
    return;

  children = clutter_container_get_children (CLUTTER_CONTAINER (view));
  children_count = g_list_length (children);

  iter = clutter_model_get_first_iter (priv->model);

  while (iter && !clutter_model_iter_is_last (iter))
  {
    model_count++;
    clutter_model_iter_next (iter);
  }

  if (children_count < model_count)
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

    clutter_model_iter_get_value (iter, 0, &value);

    /* TODO: Remove hard-coded */
    g_object_set_property ((GObject *)tile, "item", &value);
    g_value_unset (&value);
  }

  for (; l; l = l->next)
  {
    clutter_container_remove_actor (CLUTTER_CONTAINER (view), (ClutterActor *)l->data);
  }

  g_list_free (children);

  if (iter)
    g_object_unref (iter);
}

static void
_model_row_added_cb (ClutterModel     *model,
                     ClutterModelIter *iter,
                     gpointer          userdata)
{
  PengeMagicListView *view = PENGE_MAGIC_LIST_VIEW (userdata);

  penge_magic_list_view_update (view);
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

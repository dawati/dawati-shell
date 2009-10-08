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

#include <mojito-client/mojito-client-view.h>

#include "penge-people-model.h"

G_DEFINE_TYPE (PengePeopleModel, penge_people_model, CLUTTER_TYPE_LIST_MODEL)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_MODEL, PengePeopleModelPrivate))

typedef struct _PengePeopleModelPrivate PengePeopleModelPrivate;

struct _PengePeopleModelPrivate {
  MojitoClientView *view;
  guint bulk_timeout_id;
};

enum
{
  PROP_0,
  PROP_VIEW
};

enum
{
  BULK_START_SIGNAL,
  BULK_END_SIGNAL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void penge_people_model_set_view (PengePeopleModel *model,
                                         MojitoClientView *view);

static void
penge_people_model_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengePeopleModelPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_VIEW:
      g_value_set_object (value, priv->view);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_people_model_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PROP_VIEW:
      penge_people_model_set_view ((PengePeopleModel *)object,
                                   (MojitoClientView *)g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_people_model_dispose (GObject *object)
{
  PengePeopleModelPrivate *priv = GET_PRIVATE (object);

  if (priv->bulk_timeout_id)
  {
    g_source_remove (priv->bulk_timeout_id);
    priv->bulk_timeout_id = 0;
  }

  if (priv->view)
  {
    g_object_unref (priv->view);
    priv->view = NULL;
  }

  G_OBJECT_CLASS (penge_people_model_parent_class)->dispose (object);
}

static void
penge_people_model_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_model_parent_class)->finalize (object);
}

static void
penge_people_model_class_init (PengePeopleModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengePeopleModelPrivate));

  object_class->get_property = penge_people_model_get_property;
  object_class->set_property = penge_people_model_set_property;
  object_class->dispose = penge_people_model_dispose;
  object_class->finalize = penge_people_model_finalize;

  /* Register the propert to hold the view */
  pspec = g_param_spec_object ("view",
                               "View",
                               "View that this model will represent",
                               MOJITO_TYPE_CLIENT_VIEW,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_VIEW, pspec);

  signals[BULK_START_SIGNAL] =
    g_signal_new ("bulk-start",
                  PENGE_TYPE_PEOPLE_MODEL,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  signals[BULK_END_SIGNAL] =
    g_signal_new ("bulk-end",
                  PENGE_TYPE_PEOPLE_MODEL,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}

static void
penge_people_model_init (PengePeopleModel *self)
{
  GType types[] = { MOJITO_TYPE_ITEM };

  /* Set the types (or type in this case) that the model will hold */
  clutter_model_set_types (CLUTTER_MODEL (self),
                           1,
                           types);
}

ClutterModel *
penge_people_model_new (MojitoClientView *view)
{
  return g_object_new (PENGE_TYPE_PEOPLE_MODEL,
                       "view", view,
                       NULL);
}

static gint
_model_time_sort_cb (ClutterModel *model,
                     const GValue *v_a,
                     const GValue *v_b,
                     gpointer      userdata)
{
  MojitoItem *a = (MojitoItem *)g_value_get_boxed (v_a);
  MojitoItem *b = (MojitoItem *)g_value_get_boxed (v_b);

  if (a->date.tv_sec> b->date.tv_sec)
  {
    return -1;
  } else if (a->date.tv_sec == b->date.tv_sec) {
    return 0;
  } else {
    return 1;
  }
}

static gboolean
_bulk_timeout_cb (gpointer data)
{
  PengePeopleModel *model = PENGE_PEOPLE_MODEL (data);
  PengePeopleModelPrivate *priv = GET_PRIVATE (data);

  clutter_model_set_sort (CLUTTER_MODEL (model),
                        0,
                        _model_time_sort_cb,
                        NULL,
                        NULL);
  g_signal_emit (data, signals[BULK_END_SIGNAL], 0);
  priv->bulk_timeout_id = 0;

  return FALSE;
}

static void
_view_item_added_cb (MojitoClientView *view,
                     MojitoItem       *item,
                     gpointer          userdata)
{
  PengePeopleModelPrivate *priv = GET_PRIVATE (userdata);

  if (priv->bulk_timeout_id == 0)
  {
    priv->bulk_timeout_id = g_timeout_add (300,
                                           _bulk_timeout_cb,
                                           userdata);
    g_signal_emit (userdata, signals[BULK_START_SIGNAL], 0);
    clutter_model_set_sort (CLUTTER_MODEL (userdata),
                            -1,
                            NULL,
                            NULL,
                            NULL);
  } else {
    g_source_remove (priv->bulk_timeout_id);
    priv->bulk_timeout_id = g_timeout_add_seconds (1,
                                                   _bulk_timeout_cb,
                                                   userdata);
  }

  clutter_model_prepend (CLUTTER_MODEL (userdata),
                         0, item,
                         -1);
}

static void
_view_item_removed_cb (MojitoClientView *view,
                       MojitoItem       *item_in,
                       gpointer          userdata)
{
  ClutterModelIter *iter;
  MojitoItem *item;

  /* To find the item to remove from the model we must do an O(n) search
   * comparing the uuids.
   */
  iter = clutter_model_get_first_iter (CLUTTER_MODEL (userdata));

  while (!clutter_model_iter_is_last (iter))
  {
    clutter_model_iter_get (iter,
                            0, &item,
                            -1);

    if (g_str_equal (item->uuid, item_in->uuid))
    {
      clutter_model_remove (CLUTTER_MODEL (userdata),
                            clutter_model_iter_get_row (iter));
      break;
    }

    clutter_model_iter_next (iter);
  }

  g_object_unref (iter);
}

static void
penge_people_model_set_view (PengePeopleModel *model,
                             MojitoClientView *view)
{
  PengePeopleModelPrivate *priv = GET_PRIVATE (model);

  priv->view = g_object_ref (view);

  /* Connect to signals from the view and update the model */
  g_signal_connect (priv->view,
                    "item-added",
                    (GCallback)_view_item_added_cb,
                    model);
  g_signal_connect (priv->view,
                    "item-removed",
                    (GCallback)_view_item_removed_cb,
                    model);
}


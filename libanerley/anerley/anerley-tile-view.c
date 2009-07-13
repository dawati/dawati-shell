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


#include "anerley-tile-view.h"
#include <anerley/anerley-tile.h>

G_DEFINE_TYPE (AnerleyTileView, anerley_tile_view, NBTK_TYPE_ITEM_VIEW)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_TILE_VIEW, AnerleyTileViewPrivate))

typedef struct _AnerleyTileViewPrivate AnerleyTileViewPrivate;

struct _AnerleyTileViewPrivate {
  AnerleyFeedModel *model;
  ClutterActor *new_selected_actor;
  ClutterActor *selected_actor;
};

enum
{
  PROP_0,
  PROP_MODEL
};

enum
{
  ITEM_ACTIVATED,
  SELECTION_CHANGED,
  LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0 };

static void anerley_tile_view_set_model (AnerleyTileView  *view,
                                         AnerleyFeedModel *model);

#define ROW_SPACING 7
#define COL_SPACING 1

static void
anerley_tile_view_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tile_view_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PROP_MODEL:
      anerley_tile_view_set_model ((AnerleyTileView *)object,
                                   g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tile_view_dispose (GObject *object)
{
  AnerleyTileViewPrivate *priv = GET_PRIVATE (object);

  if (priv->selected_actor)
  {
    g_object_unref (priv->selected_actor);
    priv->selected_actor = NULL;
  }

  G_OBJECT_CLASS (anerley_tile_view_parent_class)->dispose (object);
}

static void
anerley_tile_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_tile_view_parent_class)->finalize (object);
}

static void
anerley_tile_view_selection_changed (AnerleyTileView *tile_view)
{
  AnerleyTileViewPrivate *priv = GET_PRIVATE (tile_view);

  /* Move the reference */
  if (priv->selected_actor)
    g_object_unref (priv->selected_actor);

  /* Move the reference, or move the NULL */
  priv->selected_actor = priv->new_selected_actor;
  priv->new_selected_actor = NULL;
}

static void
anerley_tile_view_class_init (AnerleyTileViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyTileViewPrivate));

  object_class->get_property = anerley_tile_view_get_property;
  object_class->set_property = anerley_tile_view_set_property;
  object_class->dispose = anerley_tile_view_dispose;
  object_class->finalize = anerley_tile_view_finalize;

  klass->selection_changed = anerley_tile_view_selection_changed;

  pspec = g_param_spec_object ("model",
                               "The anerley feed model",
                               "Model of feed to render",
                               ANERLEY_TYPE_FEED_MODEL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   pspec);

  signals[ITEM_ACTIVATED] =
    g_signal_new ("item-activated",
                  ANERLEY_TYPE_TILE_VIEW,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnerleyTileViewClass, item_activated),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  ANERLEY_TYPE_ITEM);

  signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  ANERLEY_TYPE_TILE_VIEW,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (AnerleyTileViewClass, selection_changed),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0,
                  G_TYPE_NONE);

}

static void
_selection_changed_before_cb (AnerleyTileView *tile_view,
                              gpointer         userdata)
{
  AnerleyTileViewPrivate *priv = GET_PRIVATE (userdata);
  ClutterActor *actor;

  actor = priv->selected_actor;

  if (actor)
    nbtk_widget_set_style_class_name ((NbtkWidget *)actor, NULL);
}

static void
_selection_changed_after_cb (AnerleyTileView *tile_view,
                             gpointer        userdata)
{
  AnerleyTileViewPrivate *priv = GET_PRIVATE (userdata);
  ClutterActor *actor;

  actor = priv->selected_actor;

  if (actor)
    nbtk_widget_set_style_class_name ((NbtkWidget *)actor, "AnerleyTileSelected");
}

static void
anerley_tile_view_set_selected_actor (AnerleyTileView *tile_view,
                                      ClutterActor    *actor)
{
  AnerleyTileViewPrivate *priv = GET_PRIVATE (tile_view);

  if (actor == priv->selected_actor)
    return;

  if (actor)
    priv->new_selected_actor = g_object_ref (actor);
  else
    priv->new_selected_actor = NULL;

  g_signal_emit (tile_view, signals[SELECTION_CHANGED], 0);
}

static void
_tile_button_release_event_cb (ClutterActor *actor,
                               ClutterEvent *event,
                               gpointer      userdata)
{
  AnerleyTileView *tile_view = (AnerleyTileView *)userdata;
  AnerleyTileViewPrivate *priv = GET_PRIVATE (tile_view);

  /* Button release event is on the same actor, unselecting it */
  if (priv->selected_actor == actor)
  {
    anerley_tile_view_set_selected_actor (tile_view, NULL);
  } else {
    anerley_tile_view_set_selected_actor (tile_view, actor);
  }
}

static void
_container_actor_added_cb (ClutterContainer *container,
                           ClutterActor     *actor,
                           gpointer          userdata)
{
  AnerleyTileView *tile_view = (AnerleyTileView *)userdata;

  g_signal_connect (actor,
                    "button-release-event",
                    (GCallback)_tile_button_release_event_cb,
                    tile_view);
}

static void
anerley_tile_view_init (AnerleyTileView *self)
{
  nbtk_item_view_set_item_type (NBTK_ITEM_VIEW (self),
                                ANERLEY_TYPE_TILE);
  nbtk_item_view_add_attribute (NBTK_ITEM_VIEW (self), "item", 0);

  g_signal_connect (self,
                    "selection-changed",
                    (GCallback)_selection_changed_before_cb,
                    self);
  g_signal_connect_after (self,
                          "selection-changed",
                          (GCallback)_selection_changed_after_cb,
                          self);

  g_signal_connect (self,
                    "actor-added",
                    (GCallback)_container_actor_added_cb,
                    self);

  nbtk_grid_set_row_gap (NBTK_GRID (self), ROW_SPACING);
  nbtk_grid_set_column_gap (NBTK_GRID (self), COL_SPACING);
}

NbtkWidget *
anerley_tile_view_new (AnerleyFeedModel *model)
{
  return g_object_new (ANERLEY_TYPE_TILE_VIEW,
                       "model",
                       model,
                       "allocate-hidden",
                       TRUE,
                       NULL);
}

static void
_bulk_change_start_cb (AnerleyFeedModel *model,
                       gpointer          userdata)
{
  /* Clear selected item, the old one is almost certainly wrong */
  anerley_tile_view_set_selected_actor ((AnerleyTileView *)userdata,
                                        NULL);
  nbtk_item_view_freeze (NBTK_ITEM_VIEW (userdata));
}

static void
_bulk_change_end_cb (AnerleyFeedModel *model,
                     gpointer          userdata)
{
  nbtk_item_view_thaw (NBTK_ITEM_VIEW (userdata));
}

static void
_model_filter_changed_cb (ClutterModel *model,
                          gpointer      userdata)
{
  /* Clear selected item, the old one is almost certainly wrong */
  anerley_tile_view_set_selected_actor ((AnerleyTileView *)userdata,
                                        NULL);
}

static void
anerley_tile_view_set_model (AnerleyTileView  *view,
                             AnerleyFeedModel *model)
{
  AnerleyTileViewPrivate *priv = GET_PRIVATE (view);
  gboolean model_was_set = FALSE;

  if (priv->model)
  {
    g_signal_handlers_disconnect_by_func (priv->model,
                                          _bulk_change_start_cb,
                                          view);
    g_signal_handlers_disconnect_by_func (priv->model,
                                          _bulk_change_end_cb,
                                          view);
    g_object_unref (priv->model);
    priv->model = NULL;
    model_was_set = TRUE;
  }

  if (model)
  {
    priv->model = g_object_ref (model);
    nbtk_item_view_set_model (NBTK_ITEM_VIEW (view),
                              (ClutterModel *)priv->model);

    g_signal_connect (model,
                      "bulk-change-start",
                      (GCallback)_bulk_change_start_cb,
                      view);

    g_signal_connect (model,
                      "bulk-change-end",
                      (GCallback)_bulk_change_end_cb,
                      view);

    g_signal_connect (model,
                      "filter-changed",
                      (GCallback)_model_filter_changed_cb,
                      view);
  } else {
    if (model_was_set)
    {
      nbtk_item_view_set_model (NBTK_ITEM_VIEW (view),
                                NULL);
    }
  }
}


AnerleyItem *
anerley_tile_view_get_selected_item (AnerleyTileView *view)
{
  AnerleyTileViewPrivate *priv = GET_PRIVATE (view);
  AnerleyItem *item;

  if (priv->selected_actor)
  {
    g_object_get (priv->selected_actor,
                  "item",
                  &item,
                  NULL);

    return item;
  } else {
    return NULL;
  }
}



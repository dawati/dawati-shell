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


#include "anerley-compact-tile-view.h"
#include <anerley/anerley-compact-tile.h>

G_DEFINE_TYPE (AnerleyCompactTileView, anerley_compact_tile_view, MX_TYPE_LIST_VIEW)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_COMPACT_TILE_VIEW, AnerleyCompactTileViewPrivate))

typedef struct _AnerleyCompactTileViewPrivate AnerleyCompactTileViewPrivate;

struct _AnerleyCompactTileViewPrivate {
  AnerleyFeedModel *model;
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

static guint signals[LAST_SIGNAL] = { 0 };

static void anerley_compact_tile_view_set_model (AnerleyCompactTileView *view,
                                                 AnerleyFeedModel       *model);

#define ROW_SPACING 7
#define COL_SPACING 1

static void
anerley_compact_tile_view_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_compact_tile_view_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  switch (property_id) {
    case PROP_MODEL:
      anerley_compact_tile_view_set_model ((AnerleyCompactTileView *)object,
                                           g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_compact_tile_view_dispose (GObject *object)
{
  G_OBJECT_CLASS (anerley_compact_tile_view_parent_class)->dispose (object);
}

static void
anerley_compact_tile_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_compact_tile_view_parent_class)->finalize (object);
}

static void
anerley_compact_tile_view_class_init (AnerleyCompactTileViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyCompactTileViewPrivate));

  object_class->get_property = anerley_compact_tile_view_get_property;
  object_class->set_property = anerley_compact_tile_view_set_property;
  object_class->dispose = anerley_compact_tile_view_dispose;
  object_class->finalize = anerley_compact_tile_view_finalize;

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
                  ANERLEY_TYPE_COMPACT_TILE_VIEW,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnerleyCompactTileViewClass, item_activated),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  ANERLEY_TYPE_ITEM);
}

static gboolean
_tile_button_release_event_cb (ClutterActor *actor,
                               ClutterEvent *event,
                               gpointer      userdata)
{
  AnerleyCompactTileView *compact_tile_view = (AnerleyCompactTileView *)userdata;

  if (((ClutterButtonEvent *)event)->click_count == 2)
  {
    AnerleyCompactTile *tile = ANERLEY_COMPACT_TILE (actor);
    AnerleyItem *item = anerley_compact_tile_get_item (tile);
    g_signal_emit_by_name (compact_tile_view,
                           "item-activated",
                           item);
  }

  return TRUE;
}

static void
_container_actor_added_cb (ClutterContainer *container,
                           ClutterActor     *actor,
                           gpointer          userdata)
{
  AnerleyCompactTileView *compact_tile_view = (AnerleyCompactTileView *)userdata;

  g_signal_connect (actor,
                    "button-release-event",
                    (GCallback)_tile_button_release_event_cb,
                    compact_tile_view);
}

static void
anerley_compact_tile_view_init (AnerleyCompactTileView *self)
{
  mx_list_view_set_item_type (MX_LIST_VIEW (self), ANERLEY_TYPE_COMPACT_TILE);
  mx_list_view_add_attribute (MX_LIST_VIEW (self), "item", 0);

  g_signal_connect (self,
                    "actor-added",
                    (GCallback)_container_actor_added_cb,
                    self);

  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), ROW_SPACING);
}

ClutterActor *
anerley_compact_tile_view_new (AnerleyFeedModel *model)
{
  return g_object_new (ANERLEY_TYPE_COMPACT_TILE_VIEW,
                       "model", model,
                       NULL);
}

static void
_bulk_change_start_cb (AnerleyFeedModel *model,
                       gpointer          userdata)
{
  mx_list_view_freeze (MX_LIST_VIEW (userdata));
}

static void
_bulk_change_end_cb (AnerleyFeedModel *model,
                     gpointer          userdata)
{
  mx_list_view_thaw (MX_LIST_VIEW (userdata));
}

static void
anerley_compact_tile_view_set_model (AnerleyCompactTileView *view,
                                     AnerleyFeedModel       *model)
{
  AnerleyCompactTileViewPrivate *priv = GET_PRIVATE (view);
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
    mx_list_view_set_model (MX_LIST_VIEW (view),
                            (ClutterModel *)priv->model);

    g_signal_connect (model,
                      "bulk-change-start",
                      (GCallback)_bulk_change_start_cb,
                      view);

    g_signal_connect (model,
                      "bulk-change-end",
                      (GCallback)_bulk_change_end_cb,
                      view);

  } else {
    if (model_was_set)
    {
      mx_list_view_set_model (MX_LIST_VIEW (view),
                              NULL);
    }
  }
}


/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <mojito-client/mojito-client.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>
#include <gconf/gconf-client.h>

#include "penge-utils.h"

#include "penge-people-tile.h"
#include "penge-people-pane.h"

#include "penge-magic-list-view.h"
#include "penge-people-model.h"

#include "penge-people-placeholder-tile.h"

G_DEFINE_TYPE (PengePeoplePane, penge_people_pane, NBTK_TYPE_WIDGET)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_PANE, PengePeoplePanePrivate))

typedef struct _PengePeoplePanePrivate PengePeoplePanePrivate;

struct _PengePeoplePanePrivate {
  MojitoClient *client;
  MojitoClientView *view;
  ClutterModel *model;
  ClutterActor *list_view;
  ClutterActor *placeholder_tile;

  gint item_count;
};

#define MAX_ITEMS 32

#define TILE_WIDTH 140.0f
#define TILE_HEIGHT 92.0f

#define MOBLIN_MYZONE_MIN_TILE_WIDTH "/desktop/moblin/myzone/min_tile_width"
#define MOBLIN_MYZONE_MIN_TILE_HEIGHT "/desktop/moblin/myzone/min_tile_height"

static void
penge_people_pane_dispose (GObject *object)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (object);

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }

  if (priv->view)
  {
    g_object_unref (priv->view);
    priv->view = NULL;
  }

  if (priv->model)
  {
    g_object_unref (priv->model);
    priv->model = NULL;
  }

  G_OBJECT_CLASS (penge_people_pane_parent_class)->dispose (object);
}

static void
penge_people_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_pane_parent_class)->finalize (object);
}

static void
_model_bulk_start_cb (ClutterModel *model,
                      gpointer      userdata)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (userdata);

  penge_magic_list_view_freeze (PENGE_MAGIC_LIST_VIEW (priv->list_view));
}

static void
_model_bulk_end_cb (ClutterModel *model,
                    gpointer      userdata)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (userdata);

  if (priv->model && clutter_model_get_n_rows (priv->model) > 0)
  {
    clutter_actor_hide (priv->placeholder_tile);
    clutter_actor_show (priv->list_view);
  } else {
    clutter_actor_hide (priv->list_view);
    clutter_actor_show (priv->placeholder_tile);
  }

  penge_magic_list_view_thaw (PENGE_MAGIC_LIST_VIEW (priv->list_view));
}

static void
_client_open_view_cb (MojitoClient     *client,
                      MojitoClientView *view,
                      gpointer          userdata)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (userdata);

  mojito_client_view_start (view);

  if (priv->model)
  {
    g_object_unref (priv->model);
    priv->model = NULL;
  }

  priv->model = penge_people_model_new (view);

  penge_magic_list_view_set_model (PENGE_MAGIC_LIST_VIEW (priv->list_view),
                                   priv->model);

  g_signal_connect (priv->model,
                    "bulk-start",
                    (GCallback)_model_bulk_start_cb,
                    userdata);
  g_signal_connect (priv->model,
                    "bulk-end",
                    (GCallback)_model_bulk_end_cb,
                    userdata);
}

static void
_client_get_services_cb (MojitoClient *client,
                         const GList  *services,
                         gpointer      userdata)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (userdata);

  mojito_client_open_view (client,
                           (GList *)services,
                           priv->item_count,
                           _client_open_view_cb,
                           userdata);

}

static void
penge_people_pane_allocate (ClutterActor          *actor,
                            const ClutterActorBox *box,
                            ClutterAllocationFlags flags)
{
  PengePeoplePane *pane = (PengePeoplePane *)actor;
  PengePeoplePanePrivate *priv = GET_PRIVATE (pane);
  gfloat width, height;
  ClutterActorBox child_box;
  gfloat placeholder_width;

  if (CLUTTER_ACTOR_CLASS (penge_people_pane_parent_class)->allocate)
    CLUTTER_ACTOR_CLASS (penge_people_pane_parent_class)->allocate (actor, box, flags);

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  child_box.y1 = 0;
  clutter_actor_get_preferred_width (priv->placeholder_tile,
                                     -1,
                                     NULL,
                                     &placeholder_width);

  child_box.x1 = (width - placeholder_width) / 2;
  child_box.x2 = child_box.x1 + placeholder_width;
  clutter_actor_get_preferred_height (priv->placeholder_tile,
                                      placeholder_width,
                                      NULL,
                                      &(child_box.y2));


  clutter_actor_allocate (priv->placeholder_tile,
                          &child_box,
                          flags);

  child_box.x1 = 0;
  child_box.x2 = width;
  child_box.y1 = 0;
  child_box.y2 = height;

  clutter_actor_allocate (priv->list_view,
                          &child_box,
                          flags);
}

static void
penge_people_pane_paint (ClutterActor *actor)
{
  PengePeoplePane *pane = (PengePeoplePane *)actor;
  PengePeoplePanePrivate *priv = GET_PRIVATE (pane);

 if (CLUTTER_ACTOR_CLASS (penge_people_pane_parent_class)->paint)
    CLUTTER_ACTOR_CLASS (penge_people_pane_parent_class)->paint (actor);

  if (CLUTTER_ACTOR_IS_MAPPED (priv->list_view))
    clutter_actor_paint (priv->list_view);

  if (CLUTTER_ACTOR_IS_MAPPED (priv->placeholder_tile))
    clutter_actor_paint (priv->placeholder_tile);
}

static void
penge_people_pane_pick (ClutterActor       *actor,
                        const ClutterColor *color)
{
  PengePeoplePane *pane = (PengePeoplePane *)actor;
  PengePeoplePanePrivate *priv = GET_PRIVATE (pane);

 if (CLUTTER_ACTOR_CLASS (penge_people_pane_parent_class)->pick)
    CLUTTER_ACTOR_CLASS (penge_people_pane_parent_class)->pick (actor, color);

  if (CLUTTER_ACTOR_IS_MAPPED (priv->list_view))
    clutter_actor_paint (priv->list_view);

  if (CLUTTER_ACTOR_IS_MAPPED (priv->placeholder_tile))
    clutter_actor_paint (priv->placeholder_tile);
}


static void
penge_people_pane_get_preferred_height (ClutterActor *self,
                                        gfloat        for_width,
                                        gfloat       *min_height_p,
                                        gfloat       *natural_height_p)
{
  if (min_height_p)
    *min_height_p = TILE_HEIGHT;

  if (natural_height_p)
    *natural_height_p = TILE_HEIGHT;
}

static void
penge_people_pane_get_preferred_width (ClutterActor *self,
                                       gfloat        for_height,
                                       gfloat       *min_width_p,
                                       gfloat       *natural_width_p)
{
  if (min_width_p)
    *min_width_p = TILE_WIDTH * 2;

  if (natural_width_p)
    *natural_width_p = TILE_WIDTH * 2;
}

static void
penge_people_pane_map (ClutterActor *actor)
{
  PengePeoplePane *pane = (PengePeoplePane *)actor;
  PengePeoplePanePrivate *priv = GET_PRIVATE (pane);

  if (CLUTTER_ACTOR_CLASS (penge_people_pane_parent_class)->map)
    CLUTTER_ACTOR_CLASS (penge_people_pane_parent_class)->map (actor);

  clutter_actor_map (priv->list_view);
  clutter_actor_map (priv->placeholder_tile);
}

static void
penge_people_pane_unmap (ClutterActor *actor)
{
  PengePeoplePane *pane = (PengePeoplePane *)actor;
  PengePeoplePanePrivate *priv = GET_PRIVATE (pane);

  if (CLUTTER_ACTOR_CLASS (penge_people_pane_parent_class)->unmap)
    CLUTTER_ACTOR_CLASS (penge_people_pane_parent_class)->unmap (actor);

  clutter_actor_unmap (priv->list_view);
  clutter_actor_unmap (priv->placeholder_tile);
}

static void
penge_people_pane_class_init (PengePeoplePaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengePeoplePanePrivate));

  object_class->dispose = penge_people_pane_dispose;
  object_class->finalize = penge_people_pane_finalize;

  actor_class->allocate = penge_people_pane_allocate;
  actor_class->paint = penge_people_pane_paint;
  actor_class->pick = penge_people_pane_pick;
  actor_class->get_preferred_width = penge_people_pane_get_preferred_width;
  actor_class->get_preferred_height = penge_people_pane_get_preferred_height;
  actor_class->map = penge_people_pane_map;
  actor_class->unmap = penge_people_pane_unmap;
}

static void
_view_count_changed_cb (PengeMagicContainer *pmc,
                        gint                 item_count,
                        gpointer             userdata)
{
  PengePeoplePane *pane = PENGE_PEOPLE_PANE (userdata);
  PengePeoplePanePrivate *priv = GET_PRIVATE (pane);

  if (item_count > priv->item_count)
  {
    priv->item_count = item_count;

    mojito_client_get_services (priv->client, _client_get_services_cb, pane);
  }
}

static void
penge_people_pane_init (PengePeoplePane *self)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (self);
  GConfClient *client;
  gfloat tile_width = 0.0, tile_height = 0.0;

  priv->client = penge_people_pane_dup_mojito_client_singleton ();

  priv->list_view = penge_magic_list_view_new ();
  g_signal_connect (priv->list_view,
                    "count-changed",
                    (GCallback)_view_count_changed_cb,
                    self);

  client = gconf_client_get_default ();

  tile_width = gconf_client_get_float (client,
                                       MOBLIN_MYZONE_MIN_TILE_WIDTH,
                                       NULL);

  /* Returns 0.0 if unset */
  if (tile_width == 0.0)
  {
    tile_width = TILE_WIDTH;
  }

  tile_height = gconf_client_get_float (client,
                                        MOBLIN_MYZONE_MIN_TILE_HEIGHT,
                                        NULL);

  if (tile_height == 0.0)
  {
    tile_height = TILE_HEIGHT;
  }

  penge_magic_container_set_minimum_child_size (PENGE_MAGIC_CONTAINER (priv->list_view),
                                                tile_width,
                                                tile_height);

  g_object_unref (client);

  penge_magic_list_view_set_item_type (PENGE_MAGIC_LIST_VIEW (priv->list_view),
                                       PENGE_TYPE_PEOPLE_TILE);
  penge_magic_list_view_add_attribute (PENGE_MAGIC_LIST_VIEW (priv->list_view),
                                       "item",
                                       0);

  priv->placeholder_tile = penge_people_placeholder_tile_new ();

  clutter_actor_hide (priv->list_view);
  clutter_actor_set_parent (priv->placeholder_tile, (ClutterActor *)self);
  clutter_actor_set_parent (priv->list_view, (ClutterActor *)self);
}

MojitoClient *
penge_people_pane_dup_mojito_client_singleton (void)
{
  static MojitoClient *client = NULL;

  if (client)
  {
    return g_object_ref (client);
  } else {
    client = mojito_client_new ();
    g_object_add_weak_pointer ((GObject *)client, (gpointer)&client);
    return client;
  }
}



/*
 * Copyright (C) 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
 */

#include <mojito-client/mojito-client.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <moblin-panel/mpl-utils.h>
#include <bickley/bkl-item-extended.h>
#include <gconf/gconf-client.h>

#include "penge-everything-pane.h"
#include "penge-recent-file-tile.h"
#include "penge-people-tile.h"
#include "penge-source-manager.h"

G_DEFINE_TYPE (PengeEverythingPane, penge_everything_pane, PENGE_TYPE_BLOCK_CONTAINER)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVERYTHING_PANE, PengeEverythingPanePrivate))

#define MOBLIN_MYZONE_MIN_TILE_WIDTH "/desktop/moblin/myzone/min_tile_width"
#define MOBLIN_MYZONE_MIN_TILE_HEIGHT "/desktop/moblin/myzone/min_tile_height"

#define TILE_WIDTH 160
#define TILE_HEIGHT 110


typedef struct _PengeEverythingPanePrivate PengeEverythingPanePrivate;

struct _PengeEverythingPanePrivate {
  MojitoClient *client;
  MojitoClientView *view;
  GtkRecentManager *recent_manager;
  GHashTable *pointer_to_actor;
  PengeSourceManager *source_manager;

  gint block_count;

  guint update_idle_id;
};

static void
penge_everything_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_everything_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_everything_pane_dispose (GObject *object)
{
  PengeEverythingPanePrivate *priv = GET_PRIVATE (object);

  if (priv->pointer_to_actor)
  {
    g_hash_table_unref (priv->pointer_to_actor);
    priv->pointer_to_actor = NULL;
  }

  if (priv->view)
  {
    g_object_unref (priv->view);
    priv->view = NULL;
  }

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }

  if (priv->source_manager)
  {
    g_object_unref (priv->source_manager);
    priv->source_manager = NULL;
  }

  if (priv->recent_manager)
  {
    g_object_unref (priv->recent_manager);
    priv->recent_manager = NULL;
  }

  G_OBJECT_CLASS (penge_everything_pane_parent_class)->dispose (object);
}

static void
penge_everything_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_everything_pane_parent_class)->finalize (object);
}

static void
penge_everything_pane_class_init (PengeEverythingPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeEverythingPanePrivate));

  object_class->get_property = penge_everything_pane_get_property;
  object_class->set_property = penge_everything_pane_set_property;
  object_class->dispose = penge_everything_pane_dispose;
  object_class->finalize = penge_everything_pane_finalize;
}

/* Sort funct for sorting recent files */
static gint
_recent_files_sort_func (GtkRecentInfo *a,
                         GtkRecentInfo *b)
{
  time_t time_a;
  time_t time_b;

  if (gtk_recent_info_get_modified (a) > gtk_recent_info_get_visited (a))
    time_a = gtk_recent_info_get_modified (a);
  else
    time_a = gtk_recent_info_get_visited (a);

  if (gtk_recent_info_get_modified (b) > gtk_recent_info_get_visited (b))
    time_b = gtk_recent_info_get_modified (b);
  else
    time_b = gtk_recent_info_get_visited (b);

  if (time_a > time_b)
  {
    return -1;
  } else if (time_a < time_b) {
    return 1;
  } else {
    return 0;
  }
}

/* Compare a MojitoItem with a GtkRecentInfo */
static gint
_compare_item_and_info (MojitoItem    *item,
                        GtkRecentInfo *info)
{
  time_t time_a;
  time_t time_b;

  /* Prefer info */
  if (item == NULL)
    return 1;

  /* Prefer item */
  if (info == NULL)
    return -1;

  time_a = item->date.tv_sec;

  if (gtk_recent_info_get_modified (info) > gtk_recent_info_get_visited (info))
    time_b = gtk_recent_info_get_modified (info);
  else
    time_b = gtk_recent_info_get_visited (info);

  if (time_a > time_b)
  {
    return -1;
  } else if (time_a < time_b) {
    return 1;
  } else {
    return 0;
  }
}

static void
_people_tile_remove_clicked_cb (PengeInterestingTile *tile,
                                gpointer              userdata)
{
  PengeEverythingPane *pane = (PengeEverythingPane *)userdata;
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);
  MojitoItem *item;

  g_object_get (tile,
                "item", &item,
                NULL);
  mojito_client_hide_item (priv->client, item);
}

static ClutterActor *
_add_from_mojito_item (PengeEverythingPane *pane,
                       MojitoItem          *item)
{
  ClutterActor *actor;

  actor = g_object_new (PENGE_TYPE_PEOPLE_TILE,
                        "item", item,
                        NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (pane), actor);

  if (penge_people_tile_is_double_size (PENGE_PEOPLE_TILE (actor)))
  {
    clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                 actor,
                                 "col-span", 2,
                                 NULL);
  }

  g_signal_connect (actor,
                    "remove-clicked",
                    (GCallback)_people_tile_remove_clicked_cb,
                    pane);

  return actor;
}

static void
_recent_file_tile_remove_clicked_cb (PengeInterestingTile *tile,
                                     gpointer              userdata)
{
  PengeEverythingPane *pane = (PengeEverythingPane *)userdata;
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);
  GtkRecentInfo *info;
  GError *error = NULL;

  g_object_get (tile,
                "info", &info,
                NULL);

  if (!gtk_recent_manager_remove_item (priv->recent_manager,
                                       gtk_recent_info_get_uri (info),
                                       &error))
  {
    g_warning (G_STRLOC ": Unable to remove item: %s",
               error->message);
    g_clear_error (&error);
  }
}

static ClutterActor *
_add_from_recent_file_info (PengeEverythingPane *pane,
                            GtkRecentInfo       *info,
                            BklItem             *bi,
                            const gchar         *thumbnail_path)
{
  ClutterActor *actor;

  actor = g_object_new (PENGE_TYPE_RECENT_FILE_TILE,
                        "info", info,
                        "item", bi,
                        "thumbnail-path", thumbnail_path,
                        NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (pane), actor);

  g_signal_connect (actor,
                    "remove-clicked",
                    (GCallback)_recent_file_tile_remove_clicked_cb,
                    pane);

  return actor;
}

static void
penge_everything_pane_update (PengeEverythingPane *pane)
{
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);
  GList *mojito_items, *recent_file_items, *l;
  GList *old_actors = NULL;
  ClutterActor *actor;

  /* Get recent files and sort */
  recent_file_items = gtk_recent_manager_get_items (priv->recent_manager);
  recent_file_items = g_list_sort (recent_file_items,
                                   (GCompareFunc)_recent_files_sort_func);

  /* Get Mojito items */
  mojito_items = mojito_client_view_get_sorted_items (priv->view);

  old_actors = g_hash_table_get_values (priv->pointer_to_actor);

  while (mojito_items || recent_file_items)
  {
    MojitoItem *mojito_item = NULL;
    GtkRecentInfo *recent_file_info = NULL;

    /* If no mojito items -> force compare to favour recent file */
    if (mojito_items)
      mojito_item = (MojitoItem *)mojito_items->data;
    else
      mojito_item = NULL;

    /* If no recent files -> force compare to favour mojito stuff */
    if (recent_file_items)
      recent_file_info = (GtkRecentInfo *)recent_file_items->data;
    else
      recent_file_info = NULL;

    if (_compare_item_and_info (mojito_item, recent_file_info) < 1)
    {
      /* Mojito item is newer */

      actor = g_hash_table_lookup (priv->pointer_to_actor,
                                   mojito_item);
      if (!actor)
      {
        actor = _add_from_mojito_item (pane, mojito_item);
        g_hash_table_insert (priv->pointer_to_actor,
                             mojito_item,
                             actor);

        /* Needed to remove from hash when we kill the actor */
        g_object_set_data (G_OBJECT (actor), "data-pointer", mojito_item);
      }

      mojito_items = g_list_remove (mojito_items, mojito_item);
    } else {
      BklItem *bi = NULL;

      /* Recent file item is newer */

      actor = g_hash_table_lookup (priv->pointer_to_actor,
                                   recent_file_info);

      if (!actor)
      {
        const gchar *uri = NULL;
        gchar *thumbnail_path = NULL;

        /* Skip *local* non-existing files */
        if (gtk_recent_info_is_local (recent_file_info) &&
          !gtk_recent_info_exists (recent_file_info))
        {
          gtk_recent_info_unref (recent_file_info);
          recent_file_items = g_list_remove (recent_file_items,
                                             recent_file_info);

          continue;
        }

        uri = gtk_recent_info_get_uri (recent_file_info);

        bi = penge_source_manager_find_item (priv->source_manager, uri);

        if (bi)
        {
          const char *thumb_uri;

          thumb_uri = bkl_item_extended_get_thumbnail ((BklItemExtended *) bi);

          if (thumb_uri)
            thumbnail_path = g_filename_from_uri (thumb_uri, NULL, NULL);
          else
            thumbnail_path = mpl_utils_get_thumbnail_path (uri);
        } else {
          thumbnail_path = mpl_utils_get_thumbnail_path (uri);
        }

        /* Skip those without thumbnail */
        if (!g_file_test (thumbnail_path, G_FILE_TEST_EXISTS))
        {
          if (bi)
            g_object_unref (bi);

          gtk_recent_info_unref (recent_file_info);
          recent_file_items = g_list_remove (recent_file_items,
                                             recent_file_info);
          g_free (thumbnail_path);
          continue;
        }

        actor = _add_from_recent_file_info (pane,
                                            recent_file_info,
                                            bi,
                                            thumbnail_path);

        g_free (thumbnail_path);
        g_hash_table_insert (priv->pointer_to_actor,
                             recent_file_info,
                             actor);

        /* Needed to remove from hash when we kill the actor */
        g_object_set_data (G_OBJECT (actor), "data-pointer", recent_file_info);
      }

      gtk_recent_info_unref (recent_file_info);
      recent_file_items = g_list_remove (recent_file_items,
                                         recent_file_info);

      if (bi)
        g_object_unref (bi);
    }

    clutter_container_lower_child (CLUTTER_CONTAINER (pane),
                                   actor,
                                   NULL);

    old_actors = g_list_remove (old_actors, actor);
  }

  for (l = old_actors; l; l = l->next)
  {
    gpointer p;
    p = g_object_get_data (G_OBJECT (l->data), "data-pointer");
    clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                    CLUTTER_ACTOR (l->data));
    g_hash_table_remove (priv->pointer_to_actor, p);
  }

  g_list_free (old_actors);

  g_list_free (mojito_items);
  g_list_free (recent_file_items);
}

static gboolean
_update_idle_cb (gpointer userdata)
{
  PengeEverythingPane *pane = (PengeEverythingPane *)userdata;
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  penge_everything_pane_update (pane);

  if (priv->block_count > 0)
  {
    /* We do this hack since we need to have something in the container to get
     * going.
     */
    clutter_actor_set_opacity (CLUTTER_ACTOR (pane), 0xff);
  }

  priv->update_idle_id = 0;

  return FALSE;
}

static void
penge_everything_pane_queue_update (PengeEverythingPane *pane)
{
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  if (priv->update_idle_id)
  {
    /* No need to update, we already have an update queued */
  } else {
    priv->update_idle_id = g_idle_add (_update_idle_cb, pane);
  }
}

static void
_view_item_added_cb (MojitoClientView *view,
                     MojitoItem       *item,
                     gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);

  penge_everything_pane_queue_update (pane);
}

static void
_view_item_removed_cb (MojitoClientView *view,
                       MojitoItem       *item,
                       gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);

  penge_everything_pane_queue_update (pane);
}

static void
_client_open_view_cb (MojitoClient     *client,
                      MojitoClientView *view,
                      gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  priv->view = view;

  g_signal_connect (view,
                    "item-added",
                    (GCallback)_view_item_added_cb,
                    userdata);
  g_signal_connect (view,
                    "item-removed",
                    (GCallback)_view_item_removed_cb,
                    userdata);
  mojito_client_view_start (view);
}

static void
_client_get_services_cb (MojitoClient *client,
                         const GList  *services,
                         gpointer      userdata)
{
  mojito_client_open_view (client,
                           (GList *)services,
                           20,
                           _client_open_view_cb,
                           userdata);
}

static void
_recent_manager_changed_cb (GtkRecentManager *manager,
                            gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);

  penge_everything_pane_queue_update (pane);
}

static void
_layout_count_changed_cb (PengeBlockContainer *pbc,
                          gint              count,
                          gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  if (priv->block_count != count)
  {
    priv->block_count = count;
    penge_everything_pane_queue_update (pane);
  }
}

static void
_source_manager_ready (PengeSourceManager *source_manager,
                       gpointer            userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);
  penge_everything_pane_queue_update (pane);
}

static void
penge_everything_pane_init (PengeEverythingPane *self)
{
  PengeEverythingPanePrivate *priv = GET_PRIVATE (self);
  gfloat tile_width, tile_height;
  GConfClient *client;

  /* pointer to pointer */
  priv->pointer_to_actor = g_hash_table_new (NULL, NULL);

  priv->client = mojito_client_new ();
  mojito_client_get_services (priv->client, _client_get_services_cb, self);

  priv->recent_manager = gtk_recent_manager_new ();

  g_signal_connect (priv->recent_manager,
                    "changed",
                    (GCallback)_recent_manager_changed_cb,
                    self);

  penge_block_container_set_spacing (PENGE_BLOCK_CONTAINER (self), 4);

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

  g_object_unref (client);

  penge_block_container_set_min_tile_size (PENGE_BLOCK_CONTAINER (self),
                                           tile_width,
                                           tile_height);

  g_signal_connect (self,
                    "count-changed",
                    (GCallback)_layout_count_changed_cb,
                    self);

  clutter_actor_set_opacity (CLUTTER_ACTOR (self), 0x0);

  /* Set up a source manager for finding the recent items */
  priv->source_manager = g_object_new (PENGE_TYPE_SOURCE_MANAGER, NULL);
  g_signal_connect (priv->source_manager,
                    "ready",
                    G_CALLBACK (_source_manager_ready),
                    self);
}


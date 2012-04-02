/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
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


#include <gtk/gtk.h>

#include "penge-apps-pane.h"
#include "penge-app-tile.h"
#include <dawati-panel/mpl-app-bookmark-manager.h>

G_DEFINE_TYPE (PengeAppsPane, penge_apps_pane, MX_TYPE_TABLE)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_APPS_PANE, PengeAppsPanePrivate))

#define GET_PRIVATE(o) ((PengeAppsPane *)o)->priv

#define TILE_COLUMN_SPACING 15
#define TILE_ROW_SPACING 10
#define TILE_STACKED_SIZE 64

struct _PengeAppsPanePrivate {
  MplAppBookmarkManager *manager;

  /* actually URIs to MxStack containing an actor with clutter name "tile */
  GHashTable *uris_to_actors;
  gboolean vertical;
};

enum
{
  PROP_0,
  PROP_VERTICAL
};

#define MAX_COUNT 8
#define ROW_SIZE 4

static void penge_apps_pane_update (PengeAppsPane *pane);

static void
penge_apps_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeAppsPanePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_VERTICAL:
      g_value_set_boolean (value, priv->vertical);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_apps_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeAppsPanePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_VERTICAL:
      priv->vertical = g_value_get_boolean (value);
      penge_apps_pane_update (PENGE_APPS_PANE (object));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_apps_pane_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_apps_pane_parent_class)->dispose (object);
}

static void
penge_apps_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_apps_pane_parent_class)->finalize (object);
}

static void
penge_apps_pane_class_init (PengeAppsPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeAppsPanePrivate));

  object_class->get_property = penge_apps_pane_get_property;
  object_class->set_property = penge_apps_pane_set_property;
  object_class->dispose = penge_apps_pane_dispose;
  object_class->finalize = penge_apps_pane_finalize;

  pspec = g_param_spec_boolean ("vertical",
                                "Vertical",
                                "Vertical mode",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_VERTICAL, pspec);
}

static void
_manager_bookmarks_changed_cb (MplAppBookmarkManager *manager,
                               gpointer               userdata)
{
  penge_apps_pane_update ((PengeAppsPane *)userdata);
}

static void
penge_apps_pane_init (PengeAppsPane *self)
{
  PengeAppsPanePrivate *priv = GET_PRIVATE_REAL (self);
  self->priv = priv;

  priv->manager = mpl_app_bookmark_manager_get_default ();

  g_signal_connect (priv->manager,
                    "bookmarks-changed",
                    (GCallback)_manager_bookmarks_changed_cb,
                    self);

  priv->uris_to_actors = g_hash_table_new_full (g_str_hash,
                                                g_str_equal,
                                                g_free,
                                                NULL);

  mx_table_set_column_spacing (MX_TABLE (self), TILE_COLUMN_SPACING);
  mx_table_set_row_spacing (MX_TABLE (self), TILE_ROW_SPACING);

  penge_apps_pane_update (self);
}

static void
penge_apps_pane_update (PengeAppsPane *pane)
{
  PengeAppsPanePrivate *priv = GET_PRIVATE (pane);
  GList *bookmarks, *l, *to_remove;
  ClutterActor *actor;
  gint count = 0;
  gchar *path;
  GError *error = NULL;
  const gchar *uri = NULL;

  bookmarks = mpl_app_bookmark_manager_get_bookmarks (priv->manager);

  to_remove = g_hash_table_get_keys (priv->uris_to_actors);

  for (l = bookmarks; l && count < MAX_COUNT; l = l->next)
  {
    uri = (gchar *)l->data;

    actor = g_hash_table_lookup (priv->uris_to_actors,
                                 uri);

    /* Check if this URI is on the system */
    path = g_filename_from_uri (uri, NULL, &error);

    if (error)
    {
      g_warning (G_STRLOC ": Error converting uri to path: %s",
                 error->message);
      g_clear_error (&error);

      if (actor)
      {
        clutter_actor_remove_child (CLUTTER_ACTOR (pane), actor);
      }

      continue;
    }

    if (!g_file_test (path, G_FILE_TEST_EXISTS))
    {
      /* skip those that have a missing .desktop file */
      g_free (path);
      continue;
    }

    g_free (path);

    if (actor)
    {
      if (!priv->vertical)
      {
        clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                     actor,
                                     "row",
                                     count / ROW_SIZE,
                                     "column",
                                     count % ROW_SIZE,
                                     NULL);
      } else {
        clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                     actor,
                                     "row",
                                     count / 1,
                                     "column",
                                     count % 1,
                                     NULL);
      }
    } else {
      ClutterActor *overlay, *stack;

      stack = mx_stack_new ();

      actor = g_object_new (PENGE_TYPE_APP_TILE,
                            "bookmark",
                            uri,
                            NULL);
      clutter_actor_set_name (actor, "tile");

      overlay = mx_frame_new ();
      mx_stylable_set_style_class (MX_STYLABLE (overlay), "PengeAppTileOverlay");

      clutter_actor_add_child (stack, actor);
      clutter_actor_add_child (stack, overlay);
      /* actor's sizes have been set in PengeAppTile */
      clutter_actor_set_size (overlay,
                              clutter_actor_get_width (actor),
                              clutter_actor_get_height (actor));
      clutter_actor_set_size (stack, TILE_STACKED_SIZE, TILE_STACKED_SIZE);

      if (!priv->vertical)
      {
        mx_table_insert_actor (MX_TABLE (pane),
                               stack,
                               count / ROW_SIZE,
                               count % ROW_SIZE);
      } else {
        mx_table_insert_actor (MX_TABLE (pane),
                               stack,
                               count / 1,
                               count % 1);
      }
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   stack,
                                   "x-expand", FALSE,
                                   "y-expand", FALSE,
                                   "x-fill", FALSE,
                                   "y-fill", FALSE,
                                   NULL);
      g_hash_table_insert (priv->uris_to_actors,
                           g_strdup (uri),
                           stack);
    }

    /* Found, so don't remove */
    /* This craziness is because the allocated string is probably different */
    to_remove = g_list_delete_link (to_remove,
                                    g_list_find_custom (to_remove,
                                                        uri,
                                                        (GCompareFunc)g_strcmp0));
    count++;
  }

  g_list_free (bookmarks);

  for (l = to_remove; l; l = g_list_delete_link (l, l))
  {
    actor = g_hash_table_lookup (priv->uris_to_actors,
                                 (gchar *)(l->data));
    clutter_actor_remove_child (CLUTTER_ACTOR (pane), actor);
    g_hash_table_remove (priv->uris_to_actors, (gchar *)(l->data));
  }
}


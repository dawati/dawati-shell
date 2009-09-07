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

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include <moblin-panel/mpl-utils.h>

#include "penge-recent-files-pane.h"
#include "penge-utils.h"
#include "penge-recent-file-tile.h"
#include "penge-recent-files-model.h"
#include "penge-magic-list-view.h"
#include "penge-welcome-tile.h"

G_DEFINE_TYPE (PengeRecentFilesPane, penge_recent_files_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_RECENT_FILES_PANE, PengeRecentFilesPanePrivate))

#define TILE_WIDTH 170
#define TILE_HEIGHT 115

#define ROW_SPACING 6
#define COL_SPACING 6

#define MOBLIN_BOOT_COUNT_KEY "/desktop/moblin/myzone/boot_count"

typedef struct _PengeRecentFilesPanePrivate PengeRecentFilesPanePrivate;

struct _PengeRecentFilesPanePrivate {
  ClutterModel *model;
  ClutterActor *welcome_tile;
  ClutterActor *list_view;
  gint boot_count;
  GtkRecentManager *manager;
};

static void
penge_recent_files_pane_dispose (GObject *object)
{
  PengeRecentFilesPanePrivate *priv = GET_PRIVATE (object);

  if (priv->model)
  {
    g_object_unref (priv->model);
    priv->model = NULL;
  }

  G_OBJECT_CLASS (penge_recent_files_pane_parent_class)->dispose (object);
}

static void
penge_recent_files_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_recent_files_pane_parent_class)->finalize (object);
}

static void
penge_recent_files_pane_get_preferred_width (ClutterActor *self,
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
penge_recent_files_pane_class_init (PengeRecentFilesPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeRecentFilesPanePrivate));

  object_class->dispose = penge_recent_files_pane_dispose;
  object_class->finalize = penge_recent_files_pane_finalize;

  actor_class->get_preferred_width = penge_recent_files_pane_get_preferred_width;
}

static void
_model_bulk_start_cb (ClutterModel *model,
                      gpointer      userdata)
{
  penge_magic_list_view_freeze (PENGE_MAGIC_LIST_VIEW (userdata));
}

static void
_model_bulk_end_cb (ClutterModel *model,
                    gpointer      userdata)
{
  penge_magic_list_view_thaw (PENGE_MAGIC_LIST_VIEW (userdata));
}

static void
_recent_manager_changed_cb (GtkRecentManager *manager,
                            gpointer          userdata)
{
  PengeRecentFilesPanePrivate *priv = GET_PRIVATE (userdata);

  GList *items, *l;

  items = gtk_recent_manager_get_items (priv->manager);

  if (g_list_length (items) > 3)
  {
    clutter_container_remove_actor (CLUTTER_CONTAINER (userdata),
                                    CLUTTER_ACTOR (priv->welcome_tile));
    clutter_container_child_set (CLUTTER_CONTAINER (userdata),
                                 CLUTTER_ACTOR (priv->list_view),
                                 "row", 0,
                                 NULL);

    g_signal_handlers_disconnect_by_func (manager,
                                          _recent_manager_changed_cb,
                                          userdata);
  }

  for (l = items; l; l = g_list_delete_link (l, l))
  {
    gtk_recent_info_unref ((GtkRecentInfo *)l->data);
  }
}

static void
penge_recent_files_pane_init (PengeRecentFilesPane *self)
{
  PengeRecentFilesPanePrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;
  GConfClient *client;
  ClutterActor *list_view;
  GList *items = NULL;
  GList *l = NULL;

  client = gconf_client_get_default ();

  priv->boot_count = gconf_client_get_int (client,
                                           MOBLIN_BOOT_COUNT_KEY,
                                           &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error getting boot count: %s",
               error->message);
    g_clear_error (&error);
  }

  if (priv->boot_count < 5)
  {
    /* increment */
    priv->boot_count++;
    if (!gconf_client_set_int (client,
                               MOBLIN_BOOT_COUNT_KEY,
                               priv->boot_count,
                               &error))
    {
      g_warning (G_STRLOC ": Error setting boot count: %s",
                 error->message);
      g_clear_error (&error);
    }
  }

  g_object_unref (client);

  list_view = penge_magic_list_view_new ();
  priv->model = penge_recent_files_model_new ();

  penge_magic_list_view_set_item_type (PENGE_MAGIC_LIST_VIEW (list_view),
                                       PENGE_TYPE_RECENT_FILE_TILE);
  penge_magic_list_view_add_attribute (PENGE_MAGIC_LIST_VIEW (list_view),
                                       "info",
                                       0);
  penge_magic_list_view_add_attribute (PENGE_MAGIC_LIST_VIEW (list_view),
                                       "thumbnail-path",
                                       1);
  penge_magic_container_set_minimum_child_size (PENGE_MAGIC_CONTAINER (list_view),
                                                TILE_WIDTH,
                                                TILE_HEIGHT);

  penge_magic_list_view_set_model (PENGE_MAGIC_LIST_VIEW (list_view),
                                   priv->model);

  g_signal_connect (priv->model,
                    "bulk-start",
                    (GCallback)_model_bulk_start_cb,
                    list_view);
  g_signal_connect (priv->model,
                    "bulk-end",
                    (GCallback)_model_bulk_end_cb,
                    list_view);

  if (priv->boot_count < 5)
  {
    priv->manager = gtk_recent_manager_get_default ();
    items = gtk_recent_manager_get_items (priv->manager);

    if (g_list_length (items) < 4)
    {
      priv->welcome_tile = penge_welcome_tile_new ();
      nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                            priv->welcome_tile,
                                            0, 0,
                                            "x-expand", FALSE,
                                            "y-expand", FALSE,
                                            "x-fill", FALSE,
                                            "y-fill", TRUE,
                                            NULL);
      nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                            list_view,
                                            1, 0,
                                            "x-expand", TRUE,
                                            "y-expand", TRUE,
                                            "x-fill", TRUE,
                                            "y-fill", TRUE,
                                            NULL);

      /* We will remove this callback if list length exceeds 3 */
      g_signal_connect (priv->manager,
                        "changed",
                        (GCallback)_recent_manager_changed_cb,
                        self);
    } else {
      priv->manager = NULL;
    }
  }

  if (!priv->welcome_tile)
  {
    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          list_view,
                                          0, 0,
                                          "x-expand", TRUE,
                                          "y-expand", TRUE,
                                          "x-fill", TRUE,
                                          "y-fill", TRUE,
                                          NULL);
  }

  priv->list_view = list_view;

  for (l = items; l; l = g_list_delete_link (l, l))
  {
    gtk_recent_info_unref ((GtkRecentInfo *)l->data);
  }
}

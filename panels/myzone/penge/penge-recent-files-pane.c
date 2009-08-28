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

G_DEFINE_TYPE (PengeRecentFilesPane, penge_recent_files_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_RECENT_FILES_PANE, PengeRecentFilesPanePrivate))

#define NUMBER_COLS 2
#define NUMBER_OF_ITEMS 8

#define TILE_WIDTH 170
#define TILE_HEIGHT 115

#define ROW_SPACING 6
#define COL_SPACING 6

#define MOBLIN_BOOT_COUNT_KEY "/desktop/moblin/myzone/boot_count"

typedef struct _PengeRecentFilesPanePrivate PengeRecentFilesPanePrivate;

struct _PengeRecentFilesPanePrivate {
  ClutterModel *model;
  ClutterActor *welcome_tile;
  gint boot_count;
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
penge_recent_files_pane_class_init (PengeRecentFilesPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeRecentFilesPanePrivate));

  object_class->dispose = penge_recent_files_pane_dispose;
  object_class->finalize = penge_recent_files_pane_finalize;
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
penge_recent_files_pane_init (PengeRecentFilesPane *self)
{
  PengeRecentFilesPanePrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;
  GConfClient *client;
  ClutterActor *list_view;

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

  /* increment */
  priv->boot_count++;

  if (priv->boot_count <= 5)
  {
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

  penge_magic_list_view_set_model (PENGE_MAGIC_LIST_VIEW (list_view), priv->model);

  g_signal_connect (priv->model,
                    "bulk-start",
                    (GCallback)_model_bulk_start_cb,
                    list_view);
  g_signal_connect (priv->model,
                    "bulk-end",
                    (GCallback)_model_bulk_end_cb,
                    list_view);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        list_view,
                                        0, 0,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        NULL);
}

#if 0
static ClutterActor *
_make_welcome_tile ()
{
  NbtkWidget *tile;
  NbtkWidget *label;
  ClutterActor *tmp_text;

  tile = nbtk_table_new ();
  clutter_actor_set_width ((ClutterActor *)tile,
                           TILE_WIDTH * 2 + COL_SPACING);
  nbtk_widget_set_style_class_name ((NbtkWidget *)tile, "PengeWelcomeTile");



  label = nbtk_label_new (_("<b>Welcome to Moblin 2.0 for Netbooks</b>"));
  clutter_actor_set_name ((ClutterActor *)label,
                          "penge-welcome-primary-text");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_use_markup (CLUTTER_TEXT (tmp_text),
                               TRUE);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (tile),
                                        (ClutterActor *)label,
                                        0,
                                        0,
                                        "x-expand",
                                        TRUE,
                                        "x-fill",
                                        TRUE,
                                        "y-expand",
                                        TRUE,
                                        "y-fill",
                                        TRUE,
                                        NULL);

  label = nbtk_label_new (_("As Moblin is a bit different to other computers, " \
                            "we've put together a couple of bits and pieces to " \
                            "help you find your way around. " \
                            "We hope you enjoy it, The Moblin Team."));
  clutter_actor_set_name ((ClutterActor *)label,
                          "penge-welcome-secondary-text");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (tile),
                                        (ClutterActor *)label,
                                        1,
                                        0,
                                        "x-expand",
                                        TRUE,
                                        "x-fill",
                                        TRUE,
                                        "y-expand",
                                        TRUE,
                                        "y-fill",
                                        TRUE,
                                        NULL);
  return (ClutterActor *)tile;
}

static void
penge_recent_files_pane_update (PengeRecentFilesPane *pane)
{
  PengeRecentFilesPanePrivate *priv = GET_PRIVATE (pane);
  GList *items;
  GList *l;
  GtkRecentInfo *info;
  ClutterActor *actor;
  gint count = 0;
  gchar *thumbnail_path;
  const gchar *uri;
  GList *old_actors;
  PengeRecentFileTile *tile;
  gchar *filename = NULL;

  items = gtk_recent_manager_get_items (priv->manager);

  if (priv->boot_count > 5 || g_list_length (items) > 3)
  {
    if (priv->welcome_tile)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                      priv->welcome_tile);
      priv->welcome_tile = NULL;
    }
  } else {
    if (!priv->welcome_tile)
    {
      priv->welcome_tile = _make_welcome_tile ();

      clutter_actor_show_all (priv->welcome_tile);
      nbtk_table_add_actor_with_properties (NBTK_TABLE (pane),
                                            priv->welcome_tile,
                                            0,
                                            0,
                                            "y-expand",
                                            FALSE,
                                            "x-expand",
                                            TRUE,
                                            NULL);
    }

    count = 2;
  }

  if (priv->welcome_tile)
  {
    if (count - 2 < 2)
    {
      /* If the number of recent file items in the pane is less than 2 then we
       * need to set the col-span to 1. The strange count - 2 < 2 is because
       * count is offset by 2 to take into consideration the welcome tile.
       * Work around a bug in NbtkTable #4686
       */
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   priv->welcome_tile,
                                   "col-span",
                                   1,
                                   NULL);
    } else {
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   priv->welcome_tile,
                                   "col-span",
                                   2,
                                   NULL);
    }
  }
}
#endif

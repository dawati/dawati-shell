#include <gtk/gtk.h>
#include <gio/gio.h>

#include "penge-recent-files-pane.h"
#include "penge-utils.h"
#include "penge-recent-file-tile.h"

G_DEFINE_TYPE (PengeRecentFilesPane, penge_recent_files_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_RECENT_FILES_PANE, PengeRecentFilesPanePrivate))

#define NUMBER_COLS 2
#define NUMBER_OF_ITEMS 8

static void penge_recent_files_pane_update (PengeRecentFilesPane *pane);

typedef struct _PengeRecentFilesPanePrivate PengeRecentFilesPanePrivate;

struct _PengeRecentFilesPanePrivate {
    GHashTable *uri_to_actor;
    GtkRecentManager *manager;
};

static void
penge_recent_files_pane_dispose (GObject *object)
{
  PengeRecentFilesPanePrivate *priv = GET_PRIVATE (object);

  if (priv->uri_to_actor)
  {
    g_hash_table_unref (priv->uri_to_actor);
    priv->uri_to_actor = NULL;
  }

  if (priv->manager)
  {
    g_object_unref (priv->manager);
    priv->manager = NULL;
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
_recent_manager_changed_cb (GtkRecentManager *manager,
                            gpointer          userdata)
{
  PengeRecentFilesPane *pane = PENGE_RECENT_FILES_PANE (userdata);

  penge_recent_files_pane_update (pane);
}

static void
penge_recent_files_pane_init (PengeRecentFilesPane *self)
{
  PengeRecentFilesPanePrivate *priv = GET_PRIVATE (self);

  priv->uri_to_actor = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              g_object_unref);

  nbtk_table_set_row_spacing (NBTK_TABLE (self), 6);
  nbtk_table_set_col_spacing (NBTK_TABLE (self), 6);

  priv->manager = gtk_recent_manager_get_default ();
  g_signal_connect (priv->manager, 
                    "changed",
                    (GCallback)_recent_manager_changed_cb, 
                    self);
  penge_recent_files_pane_update (self);
}

static gint
_recent_files_sort_func (GtkRecentInfo *a,
                         GtkRecentInfo *b)
{
  if (gtk_recent_info_get_visited (a) > gtk_recent_info_get_visited (b))
  {
    return -1;
  } else if (gtk_recent_info_get_visited (a) < gtk_recent_info_get_visited (b)) {
    return 1;
  } else {
    return 0;
  }
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
  items = g_list_sort (items, (GCompareFunc)_recent_files_sort_func);

  old_actors = g_hash_table_get_values (priv->uri_to_actor);

  for (l = items; l && count < NUMBER_OF_ITEMS; l = l->next)
  {
    info = (GtkRecentInfo *)l->data;

    uri = gtk_recent_info_get_uri (info);

    /* Check if we already have an actor for this URI if so then position */
    actor = g_hash_table_lookup (priv->uri_to_actor, uri);

    if (actor)
    {
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   actor,
                                   "row",
                                   count / NUMBER_COLS,
                                   "column",
                                   count % NUMBER_COLS,
                                   NULL);
      old_actors = g_list_remove (old_actors, actor);
    } else {
      /* 
       * We need to check for a thumbnail image, and if we have one create the
       * PengeRecentFileTile actor else skip over it
       */
      thumbnail_path = penge_utils_get_thumbnail_path (uri);


      /* 
       * *Try* and convert URI to a filename. If it's local and it doesn't
       * exist then just skip this one. If it's non local then show it.
       */

      if (g_str_has_prefix (uri, "file:/"))
      {
        filename = g_filename_from_uri (uri,
                                        NULL,
                                        NULL);

        if (filename && !g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
          continue;
        }

        g_free (filename);
      }

      if (thumbnail_path && filename)
      {
        actor = g_object_new (PENGE_TYPE_RECENT_FILE_TILE,
                              "thumbnail-path",
                              thumbnail_path,
                              "info",
                              info,
                              NULL);
        g_free (thumbnail_path);
        nbtk_table_add_actor (NBTK_TABLE (pane),
                              actor,
                              count / NUMBER_COLS,
                              count % NUMBER_COLS);
        clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                     actor,
                                     "y-expand",
                                     FALSE,
                                     "x-expand",
                                     FALSE,
                                     NULL);
        clutter_actor_set_size (actor, 170, 115);
        g_hash_table_insert (priv->uri_to_actor,
                             g_strdup (uri),
                             g_object_ref (actor));
      }
    }

    if (actor)
    {
      /* We've added / rearranged something */
      count++;
    }
  }

  for (l = items; l; l = g_list_delete_link (l, l))
  {
    info = (GtkRecentInfo *)l->data;
    gtk_recent_info_unref (info);
  }

  for (l = old_actors; l; l = g_list_delete_link (l, l))
  {
    actor = CLUTTER_ACTOR (l->data);
    tile = PENGE_RECENT_FILE_TILE (actor);

    clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                    actor);
    g_hash_table_remove (priv->uri_to_actor,
                         penge_recent_file_tile_get_uri (tile));
  }
}

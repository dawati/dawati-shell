#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include "penge-recent-files-pane.h"
#include "penge-utils.h"
#include "penge-recent-file-tile.h"

G_DEFINE_TYPE (PengeRecentFilesPane, penge_recent_files_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_RECENT_FILES_PANE, PengeRecentFilesPanePrivate))

#define NUMBER_COLS 2
#define NUMBER_OF_ITEMS 8

#define TILE_WIDTH 170
#define TILE_HEIGHT 115

#define ROW_SPACING 6
#define COL_SPACING 6

static void penge_recent_files_pane_update (PengeRecentFilesPane *pane);

typedef struct _PengeRecentFilesPanePrivate PengeRecentFilesPanePrivate;

struct _PengeRecentFilesPanePrivate {
  GHashTable *uri_to_actor;
  GtkRecentManager *manager;
  ClutterActor *welcome_tile;
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

  nbtk_table_set_row_spacing (NBTK_TABLE (self), ROW_SPACING);
  nbtk_table_set_col_spacing (NBTK_TABLE (self), COL_SPACING);

  priv->manager = gtk_recent_manager_get_default ();
  g_signal_connect (priv->manager, 
                    "changed",
                    (GCallback)_recent_manager_changed_cb, 
                    self);


  clutter_actor_set_width ((ClutterActor *)self, TILE_WIDTH * 2 + COL_SPACING);
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

static ClutterActor *
_make_welcome_tile ()
{
  NbtkWidget *tile;
  NbtkWidget *label;
  ClutterActor *tmp_text;

  tile = nbtk_table_new ();
  clutter_actor_set_size ((ClutterActor *)tile,
                          TILE_WIDTH * 2 + COL_SPACING,
                          TILE_HEIGHT);
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
                            "help you find your way around this Beta Release. " \
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

  if (g_list_length (items) > 3)
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
                                            "col-span",
                                            2,
                                            "y-expand",
                                            FALSE,
                                            "x-expand",
                                            TRUE,
                                            NULL);
    }

    /* offset the recrnt files */
    count = 2;
  }

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
                                   "col",
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

      if (thumbnail_path)
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
                                     "y-fill",
                                     FALSE,
                                     "x-fill",
                                     FALSE,
                                     "x-align",
                                     0.0,
                                     "y-align",
                                     0.0,
                                     NULL);

        clutter_actor_set_size (actor, TILE_WIDTH, TILE_HEIGHT);
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

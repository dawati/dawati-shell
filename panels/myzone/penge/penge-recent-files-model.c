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

#include <gtk/gtk.h>
#include <moblin-panel/mpl-utils.h>

#include "penge-recent-files-model.h"

G_DEFINE_TYPE (PengeRecentFilesModel, penge_recent_files_model, CLUTTER_TYPE_LIST_MODEL)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_RECENT_FILE_MODEL, PengeRecentFilesModelPrivate))

typedef struct _PengeRecentFilesModelPrivate PengeRecentFilesModelPrivate;

struct _PengeRecentFilesModelPrivate {
  GtkRecentManager *manager;
  gint max_count;
  gint changed_idle_id;
  gboolean skip_update;
};

enum
{
  BULK_START_SIGNAL,
  BULK_END_SIGNAL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
penge_recent_files_model_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_recent_files_model_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_recent_files_model_dispose (GObject *object)
{
  PengeRecentFilesModelPrivate *priv = GET_PRIVATE (object);

  if (priv->changed_idle_id)
  {
    g_source_remove (priv->changed_idle_id);
    priv->changed_idle_id = 0;
  }

  G_OBJECT_CLASS (penge_recent_files_model_parent_class)->dispose (object);
}

static void
penge_recent_files_model_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_recent_files_model_parent_class)->finalize (object);
}

static void
penge_recent_files_model_class_init (PengeRecentFilesModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeRecentFilesModelPrivate));

  object_class->get_property = penge_recent_files_model_get_property;
  object_class->set_property = penge_recent_files_model_set_property;
  object_class->dispose = penge_recent_files_model_dispose;
  object_class->finalize = penge_recent_files_model_finalize;

  signals[BULK_START_SIGNAL] =
    g_signal_new ("bulk-start",
                  PENGE_TYPE_RECENT_FILE_MODEL,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  signals[BULK_END_SIGNAL] =
    g_signal_new ("bulk-end",
                  PENGE_TYPE_RECENT_FILE_MODEL,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}


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

static void
penge_recent_files_model_update (PengeRecentFilesModel *model)
{
  PengeRecentFilesModelPrivate *priv = GET_PRIVATE (model);
  GList *items = NULL;
  GList *l = NULL;
  GtkRecentInfo *info;
  const gchar *uri;
  gchar *thumbnail_path;

  /* Unfortunately since we don't have specific information on what has
   * changed from the signal we need to empty the model and then repopulate
   * the model using a filtered, sorted, set of data
   */

  g_signal_emit (model, signals[BULK_START_SIGNAL], 0);

  while (clutter_model_get_n_rows ((ClutterModel *)model))
    clutter_model_remove ((ClutterModel *)model, 0);

  items = gtk_recent_manager_get_items (priv->manager);
  items = g_list_sort (items, (GCompareFunc)_recent_files_sort_func);

  for (l = items; l; l = l->next)
  {
    info = (GtkRecentInfo *)l->data;

    /* Skip *local* non-existing files. This is to ensure that http:// urls
     * are included.
     */
    if (gtk_recent_info_is_local (info) &&
        !gtk_recent_info_exists (info))
    {
      gtk_recent_info_unref (info);
      continue;
    }

    uri = gtk_recent_info_get_uri (info);
    thumbnail_path = mpl_utils_get_thumbnail_path (uri);

    if (!g_file_test (thumbnail_path, G_FILE_TEST_EXISTS))
    {
      gtk_recent_info_unref (info);
      g_free (thumbnail_path);
      continue;
    }

    clutter_model_append ((ClutterModel *)model,
                          0, info,
                          1, thumbnail_path,
                          2, model,
                          -1);
    g_free (thumbnail_path);
    gtk_recent_info_unref (info);
  }

  g_signal_emit (model, signals[BULK_END_SIGNAL], 0);

  g_list_free (items);
}

static gboolean
_recent_manager_changed_idle_cb (gpointer userdata)
{
  PengeRecentFilesModelPrivate *priv = GET_PRIVATE (userdata);
  priv->changed_idle_id = 0;
  penge_recent_files_model_update ((PengeRecentFilesModel *)userdata);
  return FALSE;
}

static void
_recent_manager_changed_cb (GtkRecentManager *manager,
                            gpointer          userdata)
{
  PengeRecentFilesModelPrivate *priv = GET_PRIVATE (userdata);

  if (priv->skip_update)
  {
    priv->skip_update = FALSE;
    return;
  }

  if (priv->changed_idle_id == 0)
    priv->changed_idle_id = g_idle_add_full (G_PRIORITY_LOW,
                                             _recent_manager_changed_idle_cb,
                                             userdata,
                                             NULL);
}

static void
penge_recent_files_model_init (PengeRecentFilesModel *self)
{
  PengeRecentFilesModelPrivate *priv = GET_PRIVATE (self);
  GType types[] = { GTK_TYPE_RECENT_INFO,
                    G_TYPE_STRING,
                    PENGE_TYPE_RECENT_FILE_MODEL };

  /* Set the types (or type in this case) that the model will hold */
  clutter_model_set_types (CLUTTER_MODEL (self),
                           3,
                           types);

  priv->manager = gtk_recent_manager_get_default ();
  g_signal_connect (priv->manager,
                    "changed",
                    (GCallback)_recent_manager_changed_cb,
                    self);
  penge_recent_files_model_update (self);

  /* TODO: Make updatable */
  priv->max_count = 40;
}

ClutterModel *
penge_recent_files_model_new (void)
{
  return g_object_new (PENGE_TYPE_RECENT_FILE_MODEL, NULL);
}

void
penge_recent_files_model_remove_item (PengeRecentFilesModel *model,
                                      GtkRecentInfo         *info)
{
  PengeRecentFilesModelPrivate *priv = GET_PRIVATE (model);
  GError *error = NULL;

  priv->skip_update = FALSE;

  if (!gtk_recent_manager_remove_item (priv->manager,
                                       gtk_recent_info_get_uri (info),
                                       &error))
  {
    g_warning (G_STRLOC ": Unable to remove item: %s",
               error->message);
    g_clear_error (&error);
  }

  priv->skip_update = TRUE;
}

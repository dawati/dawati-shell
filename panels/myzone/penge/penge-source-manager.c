/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Iain Holmes <iain@linux.intel.com>
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

#include <bickley/bkl-source-manager-client.h>
#include <bickley/bkl-source-client.h>

#include <bickley/bkl-item.h>

#include "penge-source-manager.h"

enum {
  READY,
  LAST_SIGNAL,
};

struct _PengeSourceManagerPrivate {
  BklSourceManagerClient *manager;

  BklSourceClient *local_source;
  GList *transient_sources;

  guint source_count; /* The number of sources we have */
  guint source_replies; /* The number of replies (success of failure) that
                           we have received */
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), PENGE_TYPE_SOURCE_MANAGER, PengeSourceManagerPrivate))

G_DEFINE_TYPE (PengeSourceManager, penge_source_manager, G_TYPE_OBJECT);

static guint32 signals[LAST_SIGNAL] = {0,};

static void
penge_source_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_source_manager_parent_class)->finalize (object);
}

static void
penge_source_manager_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_source_manager_parent_class)->dispose (object);
}

static void
penge_source_manager_class_init (PengeSourceManagerClass *klass)
{
  GObjectClass *o_class = (GObjectClass *) klass;

  o_class->dispose = penge_source_manager_dispose;
  o_class->finalize = penge_source_manager_finalize;

  g_type_class_add_private (klass, sizeof (PengeSourceManagerPrivate));

  signals[READY] = g_signal_new ("ready",
                                 G_TYPE_FROM_CLASS (klass),
                                 G_SIGNAL_RUN_FIRST |
                                 G_SIGNAL_NO_RECURSE, 0, NULL, NULL,
                                 g_cclosure_marshal_VOID__VOID,
                                 G_TYPE_NONE, 0);
}

static void
check_all_sources_ready (PengeSourceManager *manager)
{
  PengeSourceManagerPrivate *priv = manager->priv;

  if (priv->source_replies == priv->source_count &&
      priv->local_source != NULL) {
    g_signal_emit (manager, signals[READY], 0);
  }
}

static void
source_ready_cb (BklSourceClient    *source_client,
                 PengeSourceManager *manager)
{
  PengeSourceManagerPrivate *priv = manager->priv;

  priv->source_replies++;

  priv->transient_sources = g_list_prepend (priv->transient_sources,
                                            source_client);
  check_all_sources_ready (manager);
}

static void
source_added_cb (BklSourceManagerClient *client,
                 const char             *object_path,
                 PengeSourceManager     *manager)
{
  BklSourceClient *source;

  source = bkl_source_client_new (object_path);
  g_signal_connect (source, "ready",
                    G_CALLBACK (source_ready_cb), manager);
}

static void
source_removed_cb (BklSourceManagerClient *client,
                   const char             *object_path,
                   PengeSourceManager    *manager)
{
  PengeSourceManagerPrivate *priv = manager->priv;
  GList *s;

  for (s = priv->transient_sources; s; s = s->next) {
    BklSourceClient *client = s->data;

    if (g_str_equal (object_path,
                     bkl_source_client_get_path (client))) {
      g_object_unref (client);
      priv->transient_sources = g_list_delete_link (priv->transient_sources, s);
      break;
    }
  }
}

static void
local_source_ready (BklSourceClient     *client,
                    PengeSourceManager *manager)
{
  PengeSourceManagerPrivate *priv = manager->priv;

  priv->local_source = client;
  check_all_sources_ready (manager);
}

static void
get_sources_reply (BklSourceManagerClient *client,
                   GList                  *sources,
                   GError                 *error,
                   gpointer                userdata)
{
  PengeSourceManager *manager = (PengeSourceManager *) userdata;
  PengeSourceManagerPrivate *priv = manager->priv;
  BklSourceClient *ls;
  GList *s;

  if (error != NULL) {
    g_warning ("Error getting sources: %s", error->message);
    g_error_free (error);
  }

  ls = bkl_source_client_new (BKL_LOCAL_SOURCE_PATH);
  g_signal_connect (ls, "ready",
                    G_CALLBACK (local_source_ready), manager);

  for (s = sources; s; s = s->next) {
    priv->source_count++;
    source_added_cb (client, s->data, manager);
  }

  g_list_free (sources);
}

static void
bkl_manager_ready_cb (BklSourceManagerClient *manager_client,
                      PengeSourceManager    *manager)
{
  PengeSourceManagerPrivate *priv = manager->priv;

  bkl_source_manager_client_get_sources (priv->manager,
                                         get_sources_reply, manager);
}

static void
penge_source_manager_init (PengeSourceManager *self)
{
  PengeSourceManagerPrivate *priv = GET_PRIVATE (self);

  self->priv = priv;

  priv->manager = g_object_new (BKL_TYPE_SOURCE_MANAGER_CLIENT, NULL);
  g_signal_connect (priv->manager, "ready",
                    G_CALLBACK (bkl_manager_ready_cb), self);
  g_signal_connect (priv->manager, "source-added",
                    G_CALLBACK (source_added_cb), self);
  g_signal_connect (priv->manager, "source-removed",
                    G_CALLBACK (source_removed_cb), self);
}

static BklItem *
find_item (BklSourceClient *client,
           const char      *uri)
{
  BklDB *db = bkl_source_client_get_db (client);
  BklItem *item = NULL;
  GError *error = NULL;

  if (db == NULL) {
    return NULL;
  }

  item = bkl_db_get_item (db, uri, &error);
  if (error != NULL) {
    if (error->code != KOZO_DB_ERROR_KEY_NOT_FOUND) {
      g_warning ("Error getting item for %s: %s", uri,
                 error->message);

      g_error_free (error);
      return NULL;
    }

    g_error_free (error);
  }

  return item;
}

BklItem *
penge_source_manager_find_item (PengeSourceManager *self,
                                const char          *uri)
{
  PengeSourceManagerPrivate *priv = self->priv;
  GList *s;
  BklItem *item;

  if (priv->local_source == NULL) {
    /* We're not ready yet */
    return NULL;
  }

  item = find_item (priv->local_source, uri);
  if (item) {
    return item;
  }

  for (s = priv->transient_sources; s; s = s->next) {
    BklSourceClient *client = s->data;

    item = find_item (client, uri);
    if (item) {
      return item;
    }
  }

  return NULL;
}

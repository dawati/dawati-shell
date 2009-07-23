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


#include "penge-tasks-pane.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>
#include <glib/gi18n.h>

#include "penge-task-tile.h"

G_DEFINE_TYPE (PengeTasksPane, penge_tasks_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_TASKS_PANE, PengeTasksPanePrivate))

typedef struct _PengeTasksPanePrivate PengeTasksPanePrivate;

struct _PengeTasksPanePrivate {
  JanaStore *store;
  JanaStoreView *view;

  GHashTable *uid_to_tasks;
  GHashTable *uid_to_actors;

  NbtkWidget *no_tasks_bin;
};

static void penge_tasks_pane_update (PengeTasksPane *pane);

#define MAX_COUNT 2

static void
penge_tasks_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_tasks_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_tasks_pane_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_tasks_pane_parent_class)->dispose (object);
}

static void
penge_tasks_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_tasks_pane_parent_class)->finalize (object);
}

static void
penge_tasks_pane_class_init (PengeTasksPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeTasksPanePrivate));

  object_class->get_property = penge_tasks_pane_get_property;
  object_class->set_property = penge_tasks_pane_set_property;
  object_class->dispose = penge_tasks_pane_dispose;
  object_class->finalize = penge_tasks_pane_finalize;
}

static void
_store_view_added_cb (JanaStoreView *view,
                      GList         *components,
                      gpointer       userdata)
{
  PengeTasksPane *pane = (PengeTasksPane *)userdata;
  PengeTasksPanePrivate *priv = GET_PRIVATE (userdata);
  JanaComponent *component;
  GList *l;
  gchar *uid;

  for (l = components; l; l = l->next)
  {
    component = (JanaComponent *)l->data;
    uid = jana_component_get_uid (component);

    if (g_hash_table_lookup (priv->uid_to_tasks, uid) != NULL)
    {
      g_warning (G_STRLOC ": added signal for an already known uid: %s",
                 uid);
      g_free (uid);
      continue;
    }

    g_hash_table_insert (priv->uid_to_tasks,
                         g_strdup (uid),
                         g_object_ref (component));

    g_free (uid);
  }

  penge_tasks_pane_update (pane);
}

static void
_store_view_modified_cb (JanaStoreView *view,
                         GList         *components,
                         gpointer       userdata)
{
  PengeTasksPane *pane = (PengeTasksPane *)userdata;
  PengeTasksPanePrivate *priv = GET_PRIVATE (userdata);
  JanaComponent *component;
  GList *l;
  gchar *uid;
  ClutterActor *actor;

  for (l = components; l; l = l->next)
  {
    component = (JanaComponent *)l->data;
    uid = jana_component_get_uid (component);

    if (g_hash_table_lookup (priv->uid_to_tasks, uid) == NULL)
    {
      g_warning (G_STRLOC ": modified signal for an unknown uid: %s",
                 uid);
      g_free (uid);
      continue;
    }

    g_hash_table_replace (priv->uid_to_tasks,
                          g_strdup (uid),
                          g_object_ref (component));

    actor = g_hash_table_lookup (priv->uid_to_actors,
                                 uid);

    if (actor)
    {
      g_object_set (actor, "task", component, NULL);
    }

    g_free (uid);
  }

  penge_tasks_pane_update (pane);
}

static void
_store_view_removed_cb (JanaStoreView *view,
                        GList         *uids,
                        gpointer       userdata)
{
  PengeTasksPane *pane = (PengeTasksPane *)userdata;
  PengeTasksPanePrivate *priv = GET_PRIVATE (userdata);
  gchar *uid;
  GList *l;

  for (l = uids; l; l = l->next)
  {
    uid = (gchar *)l->data;

    if (!g_hash_table_remove (priv->uid_to_tasks, uid))
    {
      g_warning (G_STRLOC ": asked to remove with an unknown uid: %s",
                 uid);
    }
  }

  penge_tasks_pane_update (pane);
}

static void
_store_opened_cb (JanaStore *store,
                  gpointer   userdata)
{
  PengeTasksPane *pane = (PengeTasksPane *)userdata;
  PengeTasksPanePrivate *priv = GET_PRIVATE (pane);

  priv->view = jana_store_get_view (priv->store);
  penge_tasks_pane_update (pane);

  g_signal_connect (priv->view,
                    "added",
                    (GCallback)_store_view_added_cb,
                    pane);
  g_signal_connect (priv->view,
                    "modified",
                    (GCallback)_store_view_modified_cb,
                    pane);
  g_signal_connect (priv->view,
                    "removed",
                    (GCallback)_store_view_removed_cb,
                    pane);
  jana_store_view_start (priv->view);
}

static void
penge_tasks_pane_init (PengeTasksPane *self)
{
  PengeTasksPanePrivate *priv = GET_PRIVATE (self);

  priv->uid_to_tasks = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              g_object_unref);
  priv->uid_to_actors = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               g_free,
                                               g_object_unref);

  priv->store = jana_ecal_store_new (JANA_COMPONENT_TASK);
  g_signal_connect (priv->store,
                    "opened",
                    (GCallback)_store_opened_cb,
                    self);
  jana_store_open (priv->store);
}

gint
_tasks_list_sort_cb (gconstpointer a,
                     gconstpointer b)
{
  JanaTask *task_a, *task_b;

  task_a = (JanaTask *)a;
  task_b = (JanaTask *)b;

  /* TODO: Do more interesting things here */
  if (!jana_task_get_completed (task_a) && jana_task_get_completed (task_b))
    return -1;
  else if (!jana_task_get_completed (task_b) && jana_task_get_completed (task_a))
    return 1;
  else
    return 0;
}

static void
penge_tasks_pane_update (PengeTasksPane *pane)
{
  PengeTasksPanePrivate *priv = GET_PRIVATE (pane);
  GList *tasks;
  GList *old_actors;
  gint count = 0;
  JanaTask *task;
  GList *l;
  gchar *uid;
  ClutterActor *actor;
  JanaTask *first_task = NULL;
  NbtkWidget *label;

  old_actors = g_hash_table_get_values (priv->uid_to_actors);

  tasks = g_hash_table_get_values (priv->uid_to_tasks);
  tasks = g_list_sort (tasks, _tasks_list_sort_cb);

  if (tasks)
    first_task = (JanaTask *)tasks->data;

  if (!tasks || (first_task && jana_task_get_completed (first_task)))
  {
    if (!priv->no_tasks_bin)
    {
      label = nbtk_label_new (_("Nothing to do today"));
      priv->no_tasks_bin = nbtk_bin_new ();
      nbtk_bin_set_child (NBTK_BIN (priv->no_tasks_bin),
                          (ClutterActor *)label);
      nbtk_table_add_actor (NBTK_TABLE (pane),
                            (ClutterActor *)priv->no_tasks_bin,
                            0,
                            0);
      nbtk_widget_set_style_class_name (label,
                                        "PengeNoMoreTasksLabel");

      clutter_actor_set_height ((ClutterActor *)priv->no_tasks_bin, 46);
    }
  } else {
    if (priv->no_tasks_bin)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                      (ClutterActor *)priv->no_tasks_bin);
      priv->no_tasks_bin = NULL;
    }
  }

  for (l = tasks; l && count < MAX_COUNT; l = l->next)
  {
    task = (JanaTask *)l->data;
    uid = jana_component_get_uid (JANA_COMPONENT (task));

    /* skip those that are completed. */
    if (jana_task_get_completed (task))
    {
      /* TODO: when we sort break instead */
      continue;
    }

    actor = g_hash_table_lookup (priv->uid_to_actors,
                                 uid);
    g_free (uid);

    if (actor)
    {
      old_actors = g_list_remove (old_actors, actor);
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   actor,
                                   "row",
                                   count,
                                   "col",
                                   0,
                                   NULL);
    } else {
      actor = g_object_new (PENGE_TYPE_TASK_TILE,
                            "task",
                            task,
                            "store",
                            priv->store,
                            NULL);

      clutter_actor_set_size (actor, 216, 52);
      nbtk_table_add_actor (NBTK_TABLE (pane),
                            actor,
                            count,
                            0);
      g_hash_table_insert (priv->uid_to_actors,
                           jana_component_get_uid (JANA_COMPONENT (task)),
                           g_object_ref (actor));
    }

    count++;
  }

  /* Kill off the old actors */
  for (l = old_actors; l; l = g_list_delete_link (l, l))
  {
    actor = (ClutterActor *)l->data;
    clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                    actor);
    uid = penge_task_tile_get_uid (PENGE_TASK_TILE (actor));
    g_hash_table_remove (priv->uid_to_actors,
                         uid);
    g_free (uid);
  }

  g_list_free (tasks);
}

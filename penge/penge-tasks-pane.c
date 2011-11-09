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

#include <config.h>
#include <glib/gi18n-lib.h>

#include "penge-tasks-pane.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>

#include "penge-task-tile.h"
#include "penge-utils.h"

G_DEFINE_TYPE (PengeTasksPane, penge_tasks_pane, PENGE_TYPE_DYNAMIC_BOX)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_TASKS_PANE, PengeTasksPanePrivate))


#define GET_PRIVATE(o) ((PengeTasksPane *)o)->priv

struct _PengeTasksPanePrivate {
  JanaStore *store;
  JanaStoreView *view;

  GHashTable *uid_to_tasks;
  GHashTable *uid_to_actors;

  ClutterActor *no_tasks_bin;
};

#define TILE_WIDTH 216
#define TILE_HEIGHT 52

#define TASK_ENTRY_TEXT _("Create a new task")
#define TASK_ENTRY_BUTTON _("Add")

enum {
  PRIORITY_NONE = 0,
  PRIORITY_HIGH = 1,
  PRIORITY_MEDIUM = 5,
  PRIORITY_LOW = 9,
};

static void penge_tasks_pane_update (PengeTasksPane *pane);

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
penge_tasks_pane_class_init (PengeTasksPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeTasksPanePrivate));

  object_class->get_property = penge_tasks_pane_get_property;
  object_class->set_property = penge_tasks_pane_set_property;
  object_class->dispose = penge_tasks_pane_dispose;
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
  PengeTasksPanePrivate *priv = GET_PRIVATE_REAL (self);

  penge_utils_set_locale ();

  self->priv = priv;

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

/* Copied from koto-task-store.c */
static int
get_weight (int priority, struct icaltimetype due) {

  struct icaltimetype today;

  if (priority == PRIORITY_NONE)
    priority = PRIORITY_MEDIUM;

  if (icaltime_is_null_time (due)) {
    return priority;
  }

  today = icaltime_today ();

  /* If we're due in the past */
  if (icaltime_compare_date_only (due, today) < 0)
    return priority - 10;

  /* If it's due today */
  if (icaltime_compare_date_only(due, today) == 0)
    return priority - 5;

  /* If it's due in the next three days */
  icaltime_adjust(&today, 3, 0, 0, 0);
  if (icaltime_compare_date_only(due, today) <= 0)
    return priority - 2;

  /* If its due later than a fortnight away */
  icaltime_adjust(&today, -3 + 14, 0, 0, 0);
  if (icaltime_compare_date_only(due, today) > 0)
    return priority + 2;

  return priority;
}

static gint
_calculate_weight (JanaTask *task)
{
  struct icaltimetype *itime;
  JanaTime *time;
  gint weight;
  gint priority;

  priority = jana_task_get_priority (task);
  time = jana_task_get_due_date (task);

  if (time)
  {
    g_object_get (time,
                  "icaltime", &itime,
                  NULL);

    weight = get_weight (priority,
                         *itime);

    g_object_unref (time);
  } else {
    if (priority == PRIORITY_NONE)
      priority = PRIORITY_MEDIUM;

    weight = priority;
  }

  return weight;
}

gint
_tasks_list_sort_cb (gconstpointer a,
                     gconstpointer b)
{
  JanaTask *task_a, *task_b;
  gboolean done_a, done_b;
  gint weight_a, weight_b;
  gchar *summary_a, *summary_b;
  gint res;

  task_a = (JanaTask *)a;
  task_b = (JanaTask *)b;

  done_a = jana_task_get_completed (task_a);
  done_b = jana_task_get_completed (task_b);

  if (done_a != done_b)
    return done_a < done_b ? -1 : 1;

  weight_a = _calculate_weight (task_a);
  weight_b = _calculate_weight (task_b);

  if (weight_a != weight_b)
    return weight_a < weight_b ? -1 : 1;

  summary_a = jana_task_get_summary (task_a);
  summary_b = jana_task_get_summary (task_b);

  res = g_utf8_collate (summary_a ?: "", summary_b ?: "");

  g_free (summary_a);
  g_free (summary_b);

  return res;
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
  ClutterActor *label;
  gboolean task_displayed = FALSE;

  old_actors = g_hash_table_get_values (priv->uid_to_actors);

  tasks = g_hash_table_get_values (priv->uid_to_tasks);
  tasks = g_list_sort (tasks, _tasks_list_sort_cb);

  for (l = tasks; l; l = l->next)
  {
    task = (JanaTask *)l->data;
    uid = jana_component_get_uid (JANA_COMPONENT (task));

    actor = g_hash_table_lookup (priv->uid_to_actors,
                                 uid);
    g_free (uid);

    if (actor)
    {
      old_actors = g_list_remove (old_actors, actor);
    } else {
      actor = g_object_new (PENGE_TYPE_TASK_TILE,
                            "task", task,
                            "store", priv->store,
                            NULL);

      clutter_container_add_actor (CLUTTER_CONTAINER (pane),
                                   actor);

      g_hash_table_insert (priv->uid_to_actors,
                           jana_component_get_uid (JANA_COMPONENT (task)),
                           g_object_ref (actor));
    }
    clutter_actor_raise_top (actor);
    task_displayed = TRUE;

    if (count == 0)
    {
      mx_stylable_set_style_class (MX_STYLABLE (actor), "FirstTile");
    } else {
      mx_stylable_set_style_class (MX_STYLABLE (actor), NULL);
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

  if (!task_displayed)
  {
    if (!priv->no_tasks_bin)
    {
      label = mx_label_new_with_text (_("Nothing to do today"));
      priv->no_tasks_bin = mx_frame_new ();
      mx_bin_set_child (MX_BIN (priv->no_tasks_bin),
                          (ClutterActor *)label);
      clutter_container_add_actor (CLUTTER_CONTAINER (pane),
                                   priv->no_tasks_bin);
      mx_stylable_set_style_class (MX_STYLABLE (label),
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

  g_list_free (tasks);
}


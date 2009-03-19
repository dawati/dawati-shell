#include "penge-tasks-pane.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>

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

  NbtkWidget *no_tasks_label;
};

static void penge_tasks_pane_update (PengeTasksPane *pane);

#define MAX_COUNT 8

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

    g_debug (G_STRLOC ": added: %s", uid);

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

    g_debug (G_STRLOC ": modified: %s", uid);

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

    g_debug (G_STRLOC ": removed: %s", uid);

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

  old_actors = g_hash_table_get_values (priv->uid_to_actors);

  tasks = g_hash_table_get_values (priv->uid_to_tasks);

  if (!tasks)
  {
    if (!priv->no_tasks_label)
    {
      priv->no_tasks_label = nbtk_label_new ("No pending tasks.");
      nbtk_table_add_actor (NBTK_TABLE (pane),
                            (ClutterActor *)priv->no_tasks_label,
                            0,
                            0);
      nbtk_widget_set_style_class_name (priv->no_tasks_label,
                                        "PengeNoMoreTasksText");
    }
  } else {
    if (priv->no_tasks_label)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                      (ClutterActor *)priv->no_tasks_label);
      priv->no_tasks_label = NULL;
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
    old_actors = g_list_remove (old_actors, actor);
    g_free (uid);

    if (actor)
    {
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   actor,
                                   "row",
                                   count,
                                   "column",
                                   0,
                                   NULL);
    } else {
      actor = g_object_new (PENGE_TYPE_TASK_TILE,
                            "task",
                            task,
                            "store",
                            priv->store,
                            NULL);

      clutter_actor_set_size (actor, 216, 44);
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

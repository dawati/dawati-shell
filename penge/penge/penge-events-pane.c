#include "penge-events-pane.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>

#include "penge-event-tile.h"

G_DEFINE_TYPE (PengeEventsPane, penge_events_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVENTS_PANE, PengeEventsPanePrivate))

typedef struct _PengeEventsPanePrivate PengeEventsPanePrivate;

struct _PengeEventsPanePrivate {
    JanaStore *store;
    JanaStoreView *view;
    JanaDuration *duration;

    GHashTable *uid_to_events;
    GHashTable *uid_to_actors;
};

#define MAX_COUNT 4

static void
penge_events_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_events_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_events_pane_dispose (GObject *object)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (object);

  if (priv->uid_to_events)
  {
    g_hash_table_unref (priv->uid_to_events);
    priv->uid_to_events = NULL;
  }

  if (priv->uid_to_actors)
  {
    g_hash_table_unref (priv->uid_to_actors);
    priv->uid_to_actors = NULL;
  }

  if (priv->store)
  {
    g_object_unref (priv->store);
    priv->store = NULL;
  }

  if (priv->view)
  {
    g_object_unref (priv->view);
    priv->view = NULL;
  }

  G_OBJECT_CLASS (penge_events_pane_parent_class)->dispose (object);
}

static void
penge_events_pane_finalize (GObject *object)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (object);

  if (priv->duration)
    jana_duration_free (priv->duration);

  G_OBJECT_CLASS (penge_events_pane_parent_class)->finalize (object);
}

static void
penge_events_pane_class_init (PengeEventsPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeEventsPanePrivate));

  object_class->get_property = penge_events_pane_get_property;
  object_class->set_property = penge_events_pane_set_property;
  object_class->dispose = penge_events_pane_dispose;
  object_class->finalize = penge_events_pane_finalize;
}

static gint
_event_compare_func (gconstpointer a,
                     gconstpointer b)
{
  JanaEvent *event_a = (JanaEvent *)a;
  JanaEvent *event_b = (JanaEvent *)b;

  return jana_utils_time_compare (jana_event_get_start (event_a),
                                  jana_event_get_start (event_b),
                                  FALSE);
}

static void
penge_events_pane_update (PengeEventsPane *pane)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  GList *events;
  GList *l;
  JanaEvent *event;
  gint count = 0;
  ClutterActor *actor;
  gchar *uid;
  GList *old_actors;

  /* So we can remove the "old" actors */
  old_actors = g_hash_table_get_values (priv->uid_to_actors);

  events = g_hash_table_get_values (priv->uid_to_events);
  events = g_list_sort (events, _event_compare_func);

  for (l = events; l; l = l->next)
  {
    event = (JanaEvent *)l->data;
    uid = jana_component_get_uid (JANA_COMPONENT (event));

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
      actor = g_object_new (PENGE_TYPE_EVENT_TILE,
                            "event",
                            event,
                            NULL);

      clutter_actor_set_size (actor, 216, 44);
      nbtk_table_add_actor (NBTK_TABLE (pane),
                            actor,
                            count,
                            0);
      g_hash_table_insert (priv->uid_to_actors,
                           jana_component_get_uid (JANA_COMPONENT (event)),
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
    uid = penge_event_tile_get_uid (PENGE_EVENT_TILE (actor));
    g_hash_table_remove (priv->uid_to_actors,
                         uid);
    g_free (uid);
  }

  g_list_free (events);
}

static void
_store_view_added_cb (JanaStoreView *view,
                      GList         *components,
                      gpointer       userdata)
{
  PengeEventsPane *pane = (PengeEventsPane *)userdata;
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  GList *l;
  JanaComponent *component;
  JanaEvent *event;
  gchar *uid;

  /* Explicitly check that the event is within our duration. This is because
   * i've seen a difficult to track down bug where events outside the range
   * appear
   */

  for (l = components; l; l = l->next)
  {
    component = (JanaComponent *)l->data;

    if (jana_component_get_component_type (component) == JANA_COMPONENT_EVENT)
    {
      event = (JanaEvent *)component;

      if (jana_utils_duration_contains (priv->duration,
                                        jana_event_get_start (event)))
      {
       uid = jana_component_get_uid (component);
       g_hash_table_insert (priv->uid_to_events,
                             uid,
                             g_object_ref (event));
      }
    }
  }

  penge_events_pane_update (pane);
}

static void
_store_view_modified_cb (JanaStoreView *view,
                         GList         *components,
                         gpointer       userdata)
{
  PengeEventsPane *pane = (PengeEventsPane *)userdata;
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  GList *l;
  JanaComponent *component;
  JanaEvent *old_event;
  gchar *uid;
  ClutterActor *actor;

  for (l = components; l; l = l->next)
  {
    component = (JanaComponent *)l->data;

    uid = jana_component_get_uid (component);
    old_event = g_hash_table_lookup (priv->uid_to_events,
                                     uid);

    if (old_event)
    {
      g_hash_table_replace (priv->uid_to_events,
                            jana_component_get_uid (component),
                            g_object_ref (component));
    } else {
      g_warning (G_STRLOC ": Told to modify an unknown event with uid: %s",
                 uid);
    }

    actor = g_hash_table_lookup (priv->uid_to_actors,
                                 uid);

    if (actor)
    {
      g_object_set (actor,
                    "event",
                    component,
                    NULL);
    } else {
      g_warning (G_STRLOC ": Told to modify unknown actor.");
    }

    g_free (uid);
  }

  penge_events_pane_update (pane);
}

static void
_store_view_removed_cb (JanaStoreView *view,
                        GList         *uids,
                        gpointer       userdata)
{
  PengeEventsPane *pane = (PengeEventsPane *)userdata;
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  GList *l;
  const gchar *uid;

  for (l = uids; l; l = l->next)
  {
    uid = (const gchar *)l->data;


    if (!g_hash_table_remove (priv->uid_to_events,
                         uid))
    {
      g_warning (G_STRLOC ": Asked to remove event for unknown uid:%s",
                 uid);
    }
  }

  penge_events_pane_update (pane);
}

static void
_store_opened_cb (JanaStore *store,
                  gpointer   userdata)
{
  PengeEventsPane *pane = (PengeEventsPane *)userdata;
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  JanaTime *now;
  JanaTime *end_of_day;

  priv->view = jana_store_get_view (priv->store);

  /* Set it up to only show events from nowish until the end of the day. */
  now = jana_ecal_utils_time_now_local ();
  jana_time_set_minutes (now, 0);
  end_of_day = jana_ecal_utils_time_now_local ();
  jana_time_set_hours (end_of_day, 23);
  jana_time_set_minutes (end_of_day, 59);
  priv->duration = jana_duration_new (now, end_of_day);

  jana_store_view_set_range (priv->view, now, end_of_day);

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
penge_events_pane_init (PengeEventsPane *self)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (self);
  NbtkPadding padding = { CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8) };

  /* Create hashes to store our view membership in */
  priv->uid_to_events = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               g_free,
                                               g_object_unref);
  priv->uid_to_actors = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               g_free,
                                               g_object_unref);

  /* Create a store for events then connect a signal to be fired when the
   * store is open. We will create a view and then listen on that view for
   * changes in the contents of the view
   */
  priv->store = jana_ecal_store_new (JANA_COMPONENT_EVENT);
  g_signal_connect (priv->store,
                    "opened",
                    (GCallback)_store_opened_cb,
                    self);
  jana_store_open (priv->store);

  nbtk_widget_set_padding (NBTK_WIDGET (self), &padding);
}

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


#include "penge-events-pane.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>
#include <glib/gi18n.h>

#include "penge-event-tile.h"

G_DEFINE_TYPE (PengeEventsPane, penge_events_pane, MX_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVENTS_PANE, PengeEventsPanePrivate))

typedef struct _PengeEventsPanePrivate PengeEventsPanePrivate;

struct _PengeEventsPanePrivate {
  GHashTable *stores;
  GHashTable *views;

  JanaDuration *duration;
  JanaTime *time;

  GHashTable *uid_to_events;
  GHashTable *uid_to_actors;

  ClutterActor *no_events_bin;

  gint count;

  ESourceList *source_list;
};

enum
{
  PROP_0,
  PROP_TIME
};

typedef struct {
  JanaEvent *event;
  JanaStore *store;
} PengeEventData;

#define TILE_WIDTH 216
#define TILE_HEIGHT 52

#define APPOINTMENT_ENTRY_TEXT _("Create an appointment")
#define APPOINTMENT_ENTRY_BUTTON _("Add")

static void penge_events_pane_update_duration (PengeEventsPane *pane,
                                               JanaStoreView   *view);
static void penge_events_pane_update_durations (PengeEventsPane *pane);
static void penge_events_pane_update (PengeEventsPane *pane);

static PengeEventData *penge_event_data_new (void);
static void penge_event_data_free (PengeEventData *data);

static void
penge_events_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_TIME:
      g_value_set_object (value, priv->time);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_events_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_TIME:
      if (priv->time)
        g_object_unref (priv->time);

      priv->time = g_value_dup_object (value);
      penge_events_pane_update_durations ((PengeEventsPane *)object);
      penge_events_pane_update ((PengeEventsPane *)object);
      break;
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

  if (priv->stores)
  {
    g_hash_table_unref (priv->stores);
    priv->stores = NULL;
  }

  if (priv->views)
  {
    g_hash_table_unref (priv->views);
    priv->views = NULL;
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

static gboolean
_update_idle_cb (gpointer userdata)
{
  penge_events_pane_update (PENGE_EVENTS_PANE (userdata));
  return FALSE;
}

static void
penge_events_pane_allocate (ClutterActor          *actor,
                            const ClutterActorBox *box,
                            ClutterAllocationFlags flags)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (actor);
  gfloat height;
  gint old_count;

  if (CLUTTER_ACTOR_CLASS (penge_events_pane_parent_class)->allocate)
    CLUTTER_ACTOR_CLASS (penge_events_pane_parent_class)->allocate (actor, box, flags);

  /* Work out how many we can fit in */
  height = box->y2 - box->y1;
  old_count = priv->count;
  priv->count = height / TILE_HEIGHT;

  if (old_count != priv->count)
  {
    /* Must use a high priority idle to avoid redraw artifacts */
    g_idle_add_full (G_PRIORITY_HIGH_IDLE,
                     _update_idle_cb,
                     actor,
                     NULL);
  }
}

static void
penge_events_pane_get_preferred_height (ClutterActor *actor,
                                        gfloat        for_width,
                                        gfloat       *min_height_p,
                                        gfloat       *nat_height_p)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (actor);

  if (min_height_p)
    *min_height_p = TILE_HEIGHT;

  /* Report our natural height to be our potential maximum */
  if (nat_height_p)
    *nat_height_p = TILE_HEIGHT * g_hash_table_size (priv->uid_to_events);
}

static void
penge_events_pane_class_init (PengeEventsPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeEventsPanePrivate));

  object_class->get_property = penge_events_pane_get_property;
  object_class->set_property = penge_events_pane_set_property;
  object_class->dispose = penge_events_pane_dispose;
  object_class->finalize = penge_events_pane_finalize;

  actor_class->allocate = penge_events_pane_allocate;
  actor_class->get_preferred_height = penge_events_pane_get_preferred_height;

  pspec = g_param_spec_object ("time",
                               "The time",
                               "The time now",
                               JANA_TYPE_TIME,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_TIME, pspec);
}

static gint
_event_compare_func (gconstpointer a,
                     gconstpointer b)
{
  PengeEventData *ed_a = (PengeEventData *)a;
  PengeEventData *ed_b = (PengeEventData *)b;

  JanaEvent *event_a = ed_a->event;
  JanaEvent *event_b = ed_b->event;

  JanaTime *start_a, *start_b;
  gint retval;

  start_a = jana_event_get_start (event_a);
  start_b = jana_event_get_start (event_b);

  retval = jana_utils_time_compare (start_a,
                                    start_b,
                                    FALSE);

  g_object_unref (start_a);
  g_object_unref (start_b);

  return retval;
}

static void
penge_events_pane_update (PengeEventsPane *pane)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  GList *events;
  GList *l;
  PengeEventData *event_data;
  JanaEvent *event;
  gint count = 0;
  ClutterActor *actor;
  gchar *uid;
  GList *old_actors;
  JanaTime *on_the_hour;
  GList *window_start = NULL, *window_end = NULL;
  JanaTime *t = NULL;
  ClutterActor *label;

  g_return_if_fail (priv->time);

  /* So we can remove the "old" actors */
  old_actors = g_hash_table_get_values (priv->uid_to_actors);

  events = g_hash_table_get_values (priv->uid_to_events);
  events = g_list_sort (events, _event_compare_func);

  if (!events)
  {
    if (!priv->no_events_bin)
    {
      label = mx_label_new (_("No calendar entries this week"));
      priv->no_events_bin = mx_bin_new ();
      mx_bin_set_child (MX_BIN (priv->no_events_bin),
                        label);
      mx_table_add_actor (MX_TABLE (pane),
                          priv->no_events_bin,
                          0,
                          0);
      mx_widget_set_style_class_name (MX_WIDGET (label),
                                      "PengeNoMoreEventsLabel");

      clutter_actor_set_height (priv->no_events_bin, 46);
    }
  } else {
    if (priv->no_events_bin)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                      priv->no_events_bin);
      priv->no_events_bin = NULL;
    }
  }

  on_the_hour = jana_time_duplicate (priv->time);

  jana_time_set_minutes (on_the_hour, 0);
  jana_time_set_seconds (on_the_hour, 0);

  /* Find the first approx. of the window. We try and find the first event after
   * the current time or if we are on the same hour the current events */

  /* Ideally we should show ongoing events and stuff like that. Kinda hard. */
  window_start = events;
  for (l = events; l; l = l->next)
  {
    event_data = (PengeEventData *)l->data;
    event = (JanaEvent *)event_data->event;

    t = jana_event_get_start (event);

    if (jana_utils_time_compare (t, on_the_hour, FALSE) < 0)
    {
      if (l->next)
      {
        window_start = l->next;
      } else {
        window_start = l;
      }
    } else if (jana_utils_time_compare (t, on_the_hour, FALSE) == 0) {
      window_start = l;
      g_object_unref (t);
      break;
    } else {
      window_start = l;
      g_object_unref (t);
      break;
    }

    g_object_unref (t);
  }

  /* We have at least one thing */
  if (window_start)
    count++;

  /* Next try and find the end of the window */
  window_end = window_start;
  for (l = window_start; l && count < priv->count; l = l->next)
  {
    PengeEventData *event_data = (PengeEventData *)l->data;
    event = (JanaEvent *)event_data->event;

    if (l->next)
    {
      window_end = l->next;
      count++;
    } else {
    }
  }

  /* Try and extend the window forward */
  if (count < priv->count)
  {
    for (l = window_start; l && count < priv->count; l = l->prev)
    {
      if (l->prev)
      {
        window_start = l->prev;
        count++;
      } else {
        break;
      }
    }
  }

  count = 0;
  for (l = window_start; l; l = l->next)
  {
    event_data = (PengeEventData *)l->data;
    event = event_data->event;

    uid = jana_component_get_uid (JANA_COMPONENT (event));

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
      g_object_set (actor, "time", priv->time, NULL);
    } else {

      /* TODO: Put store in here */
      actor = g_object_new (PENGE_TYPE_EVENT_TILE,
                            "event", event,
                            "time", priv->time,
                            "store", event_data->store,
                            NULL);

      clutter_actor_set_size (actor, TILE_WIDTH, TILE_HEIGHT);
      mx_table_add_actor (MX_TABLE (pane),
                          actor,
                          count,
                          0);
      g_hash_table_insert (priv->uid_to_actors,
                           jana_component_get_uid (JANA_COMPONENT (event)),
                           g_object_ref (actor));
    }

    count++;

    if (l == window_end)
      break;
  }

  /* Kill off the old actors */
  for (l = old_actors; l; l = g_list_delete_link (l, l))
  {
    actor = (ClutterActor *)l->data;

    /* TODO: Try and find where we're getting NULL from in here.. */
    if (actor)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                      actor);
      uid = penge_event_tile_get_uid (PENGE_EVENT_TILE (actor));
      g_hash_table_remove (priv->uid_to_actors,
                           uid);
      g_free (uid);
    }
  }

  g_list_free (events);
  g_object_unref (on_the_hour);
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

  for (l = components; l; l = l->next)
  {
    component = (JanaComponent *)l->data;

    if (jana_component_get_component_type (component) == JANA_COMPONENT_EVENT)
    {
      PengeEventData *event_data;

      event = (JanaEvent *)component;

      uid = jana_component_get_uid (component);
      event_data = penge_event_data_new ();

      /* Gives us a new reference, no need to ref-up */
      event_data->store = jana_store_view_get_store (view);
      event_data->event = g_object_ref (event);

      g_hash_table_insert (priv->uid_to_events,
                           uid,
                           event_data);
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
  PengeEventData *event_data;

  for (l = components; l; l = l->next)
  {
    component = (JanaComponent *)l->data;

    uid = jana_component_get_uid (component);
    old_event = g_hash_table_lookup (priv->uid_to_events,
                                     uid);

    if (old_event)
    {
      event_data = g_hash_table_lookup (priv->uid_to_events, g_strdup (uid));
      g_object_unref (event_data->event);
      event_data->event = g_object_ref (component);
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
  JanaStoreView *view;

  view = jana_store_get_view (store);

  g_hash_table_insert (priv->views,
                       store,
                       view);

  /* Set it up to only show events from nowish until the end of the day. */
  penge_events_pane_update_duration (pane, view);

  g_signal_connect (view,
                    "added",
                    (GCallback)_store_view_added_cb,
                    pane);
  g_signal_connect (view,
                    "modified",
                    (GCallback)_store_view_modified_cb,
                    pane);
  g_signal_connect (view,
                    "removed",
                    (GCallback)_store_view_removed_cb,
                    pane);
  jana_store_view_start (view);
}

#define CALENDAR_GCONF_PREFIX  "/apps/evolution/calendar"
#define CALENDAR_GCONF_SOURCES CALENDAR_GCONF_PREFIX "/sources"

static void
penge_events_pane_setup_stores (PengeEventsPane *pane)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  GSList *groups, *sl;
  GList *old_source_uids;
  GSList *sources = NULL;
  GList *l;

  old_source_uids = g_hash_table_get_keys (priv->stores);

  groups = e_source_list_peek_groups (priv->source_list);

  for (sl = groups; sl; sl = sl->next)
  {
    ESourceGroup *group = (ESourceGroup *)sl->data;
    GSList *group_sources;

    group_sources = e_source_group_peek_sources (group);
    sources = g_slist_concat (sources, g_slist_copy (group_sources));
  }

  for (sl = sources; sl; sl = sl->next)
  {
    ESource *source = (ESource *)sl->data;
    JanaStore *store;
    const gchar *uid;

    uid = e_source_peek_uid (source);
    store = g_hash_table_lookup (priv->stores, uid);

    if (!store)
    {
      gchar *uri;

      uri = e_source_get_uri (source);
      store = jana_ecal_store_new_from_uri (uri,
                                            JANA_COMPONENT_EVENT);
      g_hash_table_insert (priv->stores,
                           g_strdup (uid),
                           store);

      g_signal_connect (store,
                        "opened",
                        (GCallback)_store_opened_cb,
                        pane);
      jana_store_open (store);
      g_free (uri);
    }

    /* Remove from the list so that we can kill old ones */
    for (l = old_source_uids; l; l = l->next)
    {
      if (g_str_equal (l->data, uid))
      {
        old_source_uids = g_list_remove (old_source_uids, l->data);
      }
    }
  }

  g_slist_free (sources);

  for (l = old_source_uids; l; l = l->next)
  {
    JanaStore *store;
    const gchar *uid = (const gchar *)l->data;
    GHashTableIter iter;
    gchar *event_uid;
    PengeEventData *event_data;

    store = g_hash_table_lookup (priv->stores, uid);
    g_hash_table_remove (priv->views, store);

    g_hash_table_iter_init (&iter, priv->uid_to_events);

    while (g_hash_table_iter_next (&iter,
                                   (gpointer)&event_uid,
                                   (gpointer)&event_data))
    {
      if (event_data->store == store)
        g_hash_table_iter_remove (&iter);
    }
  }

  penge_events_pane_update (pane);
}


static void
_source_list_changed_cb (ESourceList *source_list,
                         gpointer     userdata)
{
  penge_events_pane_setup_stores ((PengeEventsPane *)userdata);
}

static void
penge_events_pane_init (PengeEventsPane *self)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (self);

  /* Create hashes to store our view membership in */
  priv->uid_to_events = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               g_free,
                                               (GDestroyNotify)penge_event_data_free);
  priv->uid_to_actors = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               g_free,
                                               g_object_unref);

  priv->stores = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        g_free,
                                        g_object_unref);
  priv->views = g_hash_table_new_full (NULL,
                                       NULL,
                                       NULL,
                                       g_object_unref);


  priv->source_list = e_source_list_new_for_gconf_default (CALENDAR_GCONF_SOURCES);

  g_signal_connect (priv->source_list,
                    "changed",
                    (GCallback)_source_list_changed_cb,
                    self);

  penge_events_pane_setup_stores (self);
}

static void
penge_events_pane_update_duration (PengeEventsPane *pane,
                                   JanaStoreView   *view)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  JanaTime *start_of_week;
  JanaTime *end_of_week;

  start_of_week = jana_ecal_utils_time_now_local ();
  jana_time_set_hours (start_of_week, 0);
  jana_time_set_minutes (start_of_week, 0);
  jana_time_set_seconds (start_of_week, 0);
  jana_utils_time_set_start_of_week (start_of_week);
  end_of_week = jana_ecal_utils_time_now_local ();
  jana_time_set_hours (end_of_week, 23);
  jana_time_set_minutes (end_of_week, 59);
  jana_time_set_seconds (end_of_week, 59);
  jana_utils_time_set_end_of_week (end_of_week);

  if (priv->duration)
    jana_duration_free (priv->duration);

  priv->duration = jana_duration_new (start_of_week, end_of_week);

  jana_store_view_set_range (view, start_of_week, end_of_week);
}

static void
penge_events_pane_update_durations (PengeEventsPane *pane)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  GList *views, *l;

  views = g_hash_table_get_values (priv->views);

  for (l = views; l; l = l->next)
  {
    JanaStoreView *view = (JanaStoreView *)l->data;

    penge_events_pane_update_duration (pane, view);
  }

  g_list_free (views);
}

static PengeEventData *
penge_event_data_new (void)
{
  return g_slice_new0 (PengeEventData);
}

static void
penge_event_data_free (PengeEventData *data)
{
  g_object_unref (data->event);
  g_object_unref (data->store);

  g_slice_free (PengeEventData, data);
}


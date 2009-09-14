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


#include "penge-events-pane.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>
#include <glib/gi18n.h>

#include "penge-event-tile.h"

G_DEFINE_TYPE (PengeEventsPane, penge_events_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVENTS_PANE, PengeEventsPanePrivate))

typedef struct _PengeEventsPanePrivate PengeEventsPanePrivate;

struct _PengeEventsPanePrivate {
  JanaStore *store;
  JanaStoreView *view;
  JanaDuration *duration;
  JanaTime *time;

  GHashTable *uid_to_events;
  GHashTable *uid_to_actors;

  NbtkWidget *no_events_bin;

  gint count;
};

enum
{
  PROP_0,
  PROP_TIME
};

#define TILE_WIDTH 216
#define TILE_HEIGHT 52

static void penge_events_pane_update_duration (PengeEventsPane *pane);
static void penge_events_pane_update (PengeEventsPane *pane);

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
      penge_events_pane_update_duration ((PengeEventsPane *)object);
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
  JanaEvent *event_a = (JanaEvent *)a;
  JanaEvent *event_b = (JanaEvent *)b;
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
  JanaEvent *event;
  gint count = 0;
  ClutterActor *actor;
  gchar *uid;
  GList *old_actors;
  JanaTime *on_the_hour;
  GList *window_start = NULL, *window_end = NULL;
  JanaTime *t = NULL;
  NbtkWidget *label;

  g_return_if_fail (priv->time);

  /* So we can remove the "old" actors */
  old_actors = g_hash_table_get_values (priv->uid_to_actors);

  events = g_hash_table_get_values (priv->uid_to_events);
  events = g_list_sort (events, _event_compare_func);

  if (!events)
  {
    if (!priv->no_events_bin)
    {
      label = nbtk_label_new (_("No calendar entries this week"));
      priv->no_events_bin = nbtk_bin_new ();
      nbtk_bin_set_child (NBTK_BIN (priv->no_events_bin),
                          (ClutterActor *)label);
      nbtk_table_add_actor (NBTK_TABLE (pane),
                            (ClutterActor *)priv->no_events_bin,
                            0,
                            0);
      nbtk_widget_set_style_class_name (label,
                                        "PengeNoMoreEventsLabel");

      clutter_actor_set_height ((ClutterActor *)priv->no_events_bin, 46);
    }
  } else {
    if (priv->no_events_bin)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                      (ClutterActor *)priv->no_events_bin);
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
    event = (JanaEvent *)l->data;
    t = jana_event_get_start (event);

    if (jana_utils_time_compare (t, on_the_hour, FALSE) < 0)
    {
      if (l->next)
      {
        window_start = l->next;
      }
      else
      {
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
    event = (JanaEvent *)l->data;

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
    event = (JanaEvent *)l->data;
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
      actor = g_object_new (PENGE_TYPE_EVENT_TILE,
                            "event",
                            event,
                            "time",
                            priv->time,
                            "store",
                            priv->store,
                            NULL);

      clutter_actor_set_size (actor, TILE_WIDTH, TILE_HEIGHT);
      nbtk_table_add_actor (NBTK_TABLE (pane),
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


  priv->view = jana_store_get_view (priv->store);

  /* Set it up to only show events from nowish until the end of the day. */
  penge_events_pane_update_duration (pane);

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
}

static void
penge_events_pane_update_duration (PengeEventsPane *pane)
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

  if (priv->view)
    jana_store_view_set_range (priv->view, start_of_week, end_of_week);
}

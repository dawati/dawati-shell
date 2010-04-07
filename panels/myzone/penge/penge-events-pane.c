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
#include <libical/ical.h>

#include "penge-event-tile.h"

G_DEFINE_TYPE (PengeEventsPane, penge_events_pane, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVENTS_PANE, PengeEventsPanePrivate))

typedef struct _PengeEventsPanePrivate PengeEventsPanePrivate;

struct _PengeEventsPanePrivate {
  GHashTable *stores;
  GHashTable *views;

  JanaDuration *duration;
  JanaTime *time;

  GHashTable *uid_to_events_list; /* uid to list of event data structures */
  GHashTable *uid_rid_to_actors; /* uid & rid concatenated to actor */

  ClutterActor *no_events_bin;

  ESourceList *source_list;

  gboolean multiline_summary;
};

enum
{
  PROP_0,
  PROP_TIME,
  PROP_MULTILINE_SUMMARY
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
static void penge_event_data_list_free (GList *event_data_list);

static void penge_events_pane_setup_stores (PengeEventsPane *pane);

static void
penge_events_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_TIME:
      g_value_set_object (value, priv->time);
      break;
    case PROP_MULTILINE_SUMMARY:
      g_value_set_boolean (value, priv->multiline_summary);
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
  JanaTime *start_of_week;
  JanaTime *end_of_week;

  switch (property_id) {
    case PROP_TIME:
      if (priv->time)
        g_object_unref (priv->time);

      priv->time = g_value_dup_object (value);

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
  
      penge_events_pane_update_durations ((PengeEventsPane *)object);
      penge_events_pane_update ((PengeEventsPane *)object);
      break;
    case PROP_MULTILINE_SUMMARY:
      priv->multiline_summary = g_value_get_boolean (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_events_pane_dispose (GObject *object)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (object);

  if (priv->uid_to_events_list)
  {
    g_hash_table_unref (priv->uid_to_events_list);
    priv->uid_to_events_list = NULL;
  }

  if (priv->uid_rid_to_actors)
  {
    g_hash_table_unref (priv->uid_rid_to_actors);
    priv->uid_rid_to_actors = NULL;
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

static void
penge_events_pane_constructed (GObject *object)
{
  PengeEventsPane *pane = PENGE_EVENTS_PANE (object);

  penge_events_pane_setup_stores (pane);

  if (G_OBJECT_CLASS (penge_events_pane_parent_class)->constructed)
    G_OBJECT_CLASS (penge_events_pane_parent_class)->constructed (object);
}

#define SPACING 4.0
static void
penge_events_pane_allocate (ClutterActor          *actor,
                            const ClutterActorBox *box,
                            ClutterAllocationFlags flags)
{
  //PengeEventsPanePrivate *priv = GET_PRIVATE (actor);
  gfloat width, height;
  MxPadding padding = { 0, };
  ClutterActorBox child_box;
  ClutterActorBox zero_box = { 0, };
  GList *l;
  gfloat last_y;
  GList *children;

  /* FIXME: Implement container properly ourselves */

  /* Skip the parent (the MxBoxLayout) and go straight to MxWidget. This is so
   * we don't have the actors allocated twice
   */
  CLUTTER_ACTOR_CLASS (g_type_class_peek (MX_TYPE_WIDGET))->allocate (actor, box, flags);

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  last_y = padding.top;

  children = clutter_container_get_children (CLUTTER_CONTAINER (actor));

  for (l = children; l; l = l->next)
  {
    ClutterActor *child = (ClutterActor *)l->data;
    gfloat child_nat_h;

    clutter_actor_get_preferred_height (child,
                                        width - (padding.left + padding.right),
                                        NULL,
                                        &child_nat_h);
    child_box.y1 = last_y;
    child_box.x1 = padding.left;
    child_box.x2 = width - padding.right;
    child_box.y2 = child_nat_h + last_y;
    last_y = child_box.y2;

    if (last_y <= height - padding.bottom)
      clutter_actor_allocate (child, &child_box, flags);
    else
      clutter_actor_allocate (child, &zero_box, flags);

    last_y += SPACING;
  }

  g_list_free (children);
}


static void
penge_events_pane_paint (ClutterActor *actor)
{
  //PengeEventsPanePrivate *priv = GET_PRIVATE (actor);
  GList *children, *l;

  /* FIXME: Implement container properly ourselves */

  /* Skip the parent (the MxBoxLayout) and go straight to MxWidget. This is so
   * we don't have the actors painted twice
   */
  CLUTTER_ACTOR_CLASS (g_type_class_peek (MX_TYPE_WIDGET))->paint (actor);

  children = clutter_container_get_children (CLUTTER_CONTAINER (actor));

  for (l = children; l; l = l->next)
  {
    ClutterActor *child = (ClutterActor *)l->data;
    ClutterActorBox box;

    clutter_actor_get_allocation_box (child,
                                      &box);

    if (box.x2 - box.x1 > 0 && box.y2 - box.y1 > 0)
    {
      clutter_actor_paint (child);
    }
  }

  g_list_free (children);
}


static void
penge_events_pane_pick (ClutterActor       *actor,
                        const ClutterColor *color)
{
  //PengeEventsPanePrivate *priv = GET_PRIVATE (actor);
  GList *children, *l;

  /* FIXME: Implement container properly ourselves */

  /* Skip the parent (the MxBoxLayout) and go straight to MxWidget. This is so
   * we don't have the actors painted twice
   */
  CLUTTER_ACTOR_CLASS (g_type_class_peek (MX_TYPE_WIDGET))->pick (actor, color);

  children = clutter_container_get_children (CLUTTER_CONTAINER (actor));

  for (l = children; l; l = l->next)
  {
    ClutterActor *child = (ClutterActor *)l->data;
    ClutterActorBox box;

    clutter_actor_get_allocation_box (child,
                                      &box);

    if (box.x2 - box.x1 > 0 && box.y2 - box.y1 > 0)
    {
      clutter_actor_paint (child);
    }
  }

  g_list_free (children);
}


static gfloat
_calculate_children_height (PengeEventsPane *pane,
                            gfloat           for_width)
{
  GList *l, *children;
  MxPadding padding = { 0, };
  gfloat height = 0, child_nat_h;

  mx_widget_get_padding (MX_WIDGET (pane), &padding);
  height += padding.top + padding.bottom;

  children = clutter_container_get_children (CLUTTER_CONTAINER (pane));

  for (l = children; l; l = l->next)
  {
    ClutterActor *child = (ClutterActor *)l->data;

    clutter_actor_get_preferred_height (child,
                                        for_width,
                                        NULL,
                                        &child_nat_h);

    height += child_nat_h;

    /* No SPACING on last */
    if (l->next != NULL)
      height += SPACING;
  }

  return height;
}

static void
penge_events_pane_get_preferred_height (ClutterActor *actor,
                                        gfloat        for_width,
                                        gfloat       *min_height_p,
                                        gfloat       *nat_height_p)
{
  PengeEventsPane *pane = PENGE_EVENTS_PANE (actor);

  if (min_height_p)
    *min_height_p = 0;

  /* Report our natural height to be our potential maximum */
  if (nat_height_p)
  {
    *nat_height_p = _calculate_children_height (pane,
                                                for_width);
  }
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
  object_class->constructed = penge_events_pane_constructed;

  actor_class->allocate = penge_events_pane_allocate;
  actor_class->paint = penge_events_pane_paint;
  actor_class->pick = penge_events_pane_pick;
  actor_class->get_preferred_height = penge_events_pane_get_preferred_height;

  pspec = g_param_spec_object ("time",
                               "The time",
                               "The time now",
                               JANA_TYPE_TIME,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_TIME, pspec);

  pspec = g_param_spec_boolean ("multiline-summary",
                                "Multiple line summary",
                                "Whether event summaries should use "
                                "multiple lines",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_MULTILINE_SUMMARY, pspec);
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

static GList *
penge_events_pane_flatten_events (PengeEventsPane *pane)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  GList *events = NULL, *l;
  GList *event_lists;

  event_lists = g_hash_table_get_values (priv->uid_to_events_list);

  for (l = event_lists; l; l = l->next)
  {
    /* Have to copy the list to avoid upsetting it */
    events = g_list_concat (events, g_list_copy ((GList *)(l->data)));
  }

  g_list_free (event_lists);

  return events;
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
  gchar *uid, *rid, *uid_rid;
  GList *old_actors;
  JanaTime *on_the_hour;
  JanaTime *t = NULL;
  ClutterActor *label;

  g_return_if_fail (priv->time);

  /* So we can remove the "old" actors */
  old_actors = g_hash_table_get_values (priv->uid_rid_to_actors);

  events = penge_events_pane_flatten_events (pane);
  events = g_list_sort (events, _event_compare_func);

  if (!events)
  {
    if (!priv->no_events_bin)
    {
      label = mx_label_new_with_text (_("No calendar entries this week"));
      priv->no_events_bin = mx_frame_new ();
      mx_bin_set_child (MX_BIN (priv->no_events_bin),
                        label);
      clutter_container_add_actor (CLUTTER_CONTAINER (pane),
                                   priv->no_events_bin);
      mx_stylable_set_style_class (MX_STYLABLE (label),
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

  count = 0;
  for (l = events; l; l = l->next)
  {
    event_data = (PengeEventData *)l->data;
    event = event_data->event;

    /* Skip events that are already finished */
    t = jana_event_get_end (event);
    if (jana_utils_time_compare (t, on_the_hour, FALSE) < 0)
    {
      g_object_unref (t);
      continue;
    }
    g_object_unref (t);

    uid = jana_component_get_uid (JANA_COMPONENT (event));
    rid = jana_ecal_component_get_recurrence_id (JANA_ECAL_COMPONENT (event));

    uid_rid = g_strdup_printf ("%s %s", uid, rid);

    g_free (uid);
    g_free (rid);


    actor = g_hash_table_lookup (priv->uid_rid_to_actors,
                                 uid_rid);

    if (actor)
    {
      old_actors = g_list_remove (old_actors, actor);

      g_object_set (actor, "time", priv->time, NULL);

      /* Free this one here. Other code path takes ownership */
      g_free (uid_rid);
    } else {
      actor = g_object_new (PENGE_TYPE_EVENT_TILE,
                            "event", event,
                            "time", priv->time,
                            "store", event_data->store,
                            "multiline-summary", priv->multiline_summary,
                            NULL);

      clutter_container_add_actor (CLUTTER_CONTAINER (pane),
                                   actor);

      g_hash_table_insert (priv->uid_rid_to_actors,
                           uid_rid, /* Takes ownership */
                           g_object_ref (actor));
    }
    clutter_actor_raise_top (actor);


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

    /* TODO: Try and find where we're getting NULL from in here.. */
    if (actor)
    {
      JanaComponent *event;
      clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                      actor);

      g_object_get (actor,
                    "event", &event,
                    NULL);

      uid = jana_component_get_uid (event);
      rid = jana_ecal_component_get_recurrence_id (JANA_ECAL_COMPONENT (event));

      uid_rid = g_strdup_printf ("%s %s", uid, rid);
      g_free (uid);
      g_free (rid);

      g_hash_table_remove (priv->uid_rid_to_actors,
                           uid_rid);

      g_free (uid_rid);
    }
  }

  g_list_free (events);
  g_object_unref (on_the_hour);
}

typedef struct
{
  JanaStore *store;
  GList *events_list;
} PengeRecurrenceClosure;

static gboolean
_recur_instance_generate_func (ECalComponent *ecomp,
                               time_t         start,
                               time_t         end,
                               gpointer       data)
{
  PengeRecurrenceClosure *closure = (PengeRecurrenceClosure *)data;
  PengeEventData *event_data;
  JanaEvent *jevent;
  JanaTime *stime, *etime;
  ECalComponentRange erange;

  jevent = jana_ecal_event_new_from_ecalcomp (ecomp);

  event_data = penge_event_data_new ();
  event_data->store = g_object_ref (closure->store);
  event_data->event = jevent;

  e_cal_component_get_recurid (ecomp, &erange);

  stime = jana_ecal_time_new_from_ecaltime (&(erange.datetime));
  etime = jana_time_duplicate (stime);

  jana_utils_time_adjust (etime, 0, 0, 0, 0, 0, end - start);

  jana_event_set_start (jevent, stime);
  jana_event_set_end (jevent, etime);

  closure->events_list = g_list_append (closure->events_list, event_data);

  return TRUE;
}

static GList *
_create_event_list_for_event (PengeEventsPane *pane,
                              JanaComponent   *component,
                              JanaStore       *store)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  GList *events_list = NULL;

  if (!jana_event_has_recurrence (JANA_EVENT (component)))
  {
    PengeEventData *event_data;
    JanaEvent *event;

    event = (JanaEvent *)component;

    event_data = penge_event_data_new ();

    /* Gives us a new reference, no need to ref-up */
    event_data->event = g_object_ref (event);
    event_data->store = g_object_ref (store);
    events_list = g_list_append (events_list, event_data);
  } else {
    ECalComponent *ecomp;
    ECal *ecal;
    icalcomponent *icomp;
    time_t tt_start, tt_end;
    PengeRecurrenceClosure closure;

    g_object_get (component,
                  "ecalcomp", &ecomp,
                  NULL);

    g_object_get (store,
                  "ecal", &ecal,
                  NULL);

    icomp = e_cal_component_get_icalcomponent (ecomp);

    tt_start = jana_ecal_time_to_time_t ((JanaEcalTime *)priv->duration->start);
    tt_end = jana_ecal_time_to_time_t ((JanaEcalTime *)priv->duration->end);

    closure.store = store;
    closure.events_list = NULL;

    e_cal_generate_instances_for_object (ecal,
                                         icomp,
                                         tt_start,
                                         tt_end,
                                         _recur_instance_generate_func,
                                         &closure);
    g_object_unref (ecomp);
    events_list = closure.events_list;
  }

  return events_list;
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
  gchar *uid;
  JanaStore *store;
  GList *events_list;

  store = jana_store_view_get_store (view);

  for (l = components; l; l = l->next)
  {
    component = (JanaComponent *)l->data;

    if (jana_component_get_component_type (component) == JANA_COMPONENT_EVENT)
    {
      uid = jana_component_get_uid (component);
      events_list = _create_event_list_for_event (pane, component, store);
      g_hash_table_insert (priv->uid_to_events_list,
                           uid,
                           events_list);
    }
  }

  g_object_unref (store);

  penge_events_pane_update (pane);
}

static void
_store_view_modified_cb (JanaStoreView *view,
                         GList         *components,
                         gpointer       userdata)
{
  PengeEventsPane *pane = (PengeEventsPane *)userdata;
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
  GList *l, *ll;
  JanaComponent *component;
  gchar *uid;
  ClutterActor *actor;
  GList *old_events_list = NULL;
  GList *events_list = NULL;
  JanaStore *store;

  store = jana_store_view_get_store (view);

  for (l = components; l; l = l->next)
  {
    component = (JanaComponent *)l->data;

    uid = jana_component_get_uid (component);
    old_events_list = g_hash_table_lookup (priv->uid_to_events_list,
                                           uid);

    if (old_events_list)
    {
      events_list = _create_event_list_for_event (pane, component, store);
      g_hash_table_remove (priv->uid_to_events_list, uid);
      g_hash_table_insert (priv->uid_to_events_list,
                           g_strdup (uid),
                           events_list);
    } else {
      /* Our range contains events that we might not have actors for */
    }

    for (ll = events_list; ll; ll = ll->next)
    {
      PengeEventData *event_data = (PengeEventData *)ll->data;
      JanaComponent *comp = (JanaComponent *)event_data->event;
      gchar *rid;
      gchar *uid_rid;

      rid = jana_ecal_component_get_recurrence_id (JANA_ECAL_COMPONENT (comp));

      uid_rid = g_strdup_printf ("%s %s", uid, rid);
      g_free (rid);

      actor = g_hash_table_lookup (priv->uid_rid_to_actors, uid_rid);

      if (actor)
      {
        g_object_set (actor,
                      "event", component,
                      NULL);
      } else {
        /* Our range contains events we might not have actors for */
      }

      g_free (uid_rid);
    }

    g_free (uid);
  }

  g_object_unref (store);

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

    if (!g_hash_table_remove (priv->uid_to_events_list,
                              uid))
    {
      /* Our range might contain events that we aren't presenting */
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
      ECal *ecal;

      /* Do this to support sources that have more that a URI */
      ecal = e_cal_new (source, E_CAL_SOURCE_TYPE_EVENT);
      store = g_object_new (JANA_ECAL_TYPE_STORE,
                            "ecal", ecal,
                            "type", JANA_COMPONENT_EVENT,
                            NULL);
      g_object_unref (ecal);


      g_hash_table_insert (priv->stores,
                           g_strdup (uid),
                           store);

      g_signal_connect (store,
                        "opened",
                        (GCallback)_store_opened_cb,
                        pane);
      jana_store_open (store);
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
    GList *events_list = NULL;
    PengeEventData *event_data;
    GList *l;

    store = g_hash_table_lookup (priv->stores, uid);
    g_hash_table_remove (priv->views, store);

    g_hash_table_iter_init (&iter, priv->uid_to_events_list);

    while (g_hash_table_iter_next (&iter,
                                   (gpointer)&event_uid,
                                   (gpointer)&events_list))
    {
      for (l = events_list; l; l = l->next)
      {
        event_data = (PengeEventData *)l->data;
        if (event_data->store == store)
          g_hash_table_iter_remove (&iter);
      }
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
  priv->uid_to_events_list = g_hash_table_new_full (g_str_hash,
                                                    g_str_equal,
                                                    g_free,
                                                    (GDestroyNotify)penge_event_data_list_free);
  priv->uid_rid_to_actors = g_hash_table_new_full (g_str_hash,
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
}

static void
penge_events_pane_update_duration (PengeEventsPane *pane,
                                   JanaStoreView   *view)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);


  jana_store_view_set_range (view, priv->duration->start, priv->duration->end);
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

static void
penge_event_data_list_free (GList *event_data_list)
{
  PengeEventData *event_data;
  GList *l;

  for (l = event_data_list; l; l = l->next)
  {
    event_data = (PengeEventData *)l->data;
    penge_event_data_free (event_data);
  }
}

void 
penge_events_pane_set_duration (PengeEventsPane *pane, JanaDuration *duration)
{
  PengeEventsPanePrivate *priv = GET_PRIVATE (pane);
	
  if (priv->duration)
    jana_duration_free (priv->duration);

  priv->duration = jana_duration_new (duration->start, duration->end);
  if (priv->time)
    g_object_unref (priv->time);
  priv->time = g_object_ref(duration->start);

  penge_events_pane_update_durations (pane);
}


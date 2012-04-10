/*
 * Copyright (C) 2009 Intel Corporation.
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

#include <zeitgeist.h>
#include <libsocialweb-client/sw-client.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <dawati-panel/mpl-utils.h>
#include <gconf/gconf-client.h>

#include "penge-everything-pane.h"
#include "penge-recent-file-tile.h"
#include "penge-people-tile.h"

#include "penge-welcome-tile.h"

G_DEFINE_TYPE (PengeEverythingPane, penge_everything_pane, PENGE_TYPE_BLOCK_CONTAINER)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVERYTHING_PANE, PengeEverythingPanePrivate))

#define GET_PRIVATE(o) ((PengeEverythingPane *)o)->priv;

#define DAWATI_MYZONE_MIN_TILE_WIDTH "/desktop/dawati/myzone/min_tile_width"
#define DAWATI_MYZONE_MIN_TILE_HEIGHT "/desktop/dawati/myzone/min_tile_height"
#define DAWATI_MYZONE_RATIO "/desktop/dawati/myzone/ratio"

#define TILE_WIDTH 164
#define TILE_HEIGHT 170
#define REFRESH_TIME (600) /* 10 minutes */

static void _zeitgeist_monitor_events_inserted_signal (ZeitgeistMonitor *m,
      ZeitgeistTimeRange *time_range,
      GPtrArray *events,
      gpointer          userdata);
static void _zeitgeist_monitor_events_deleted_signal (ZeitgeistMonitor *m,
    ZeitgeistTimeRange *time_range,
    GArray *ids,
    gpointer userdata);



struct _PengeEverythingPanePrivate {
  SwClient *client;
  GList *views;
  ZeitgeistLog *recent_log;
  ZeitgeistMonitor *recent_monitor;
  GHashTable *pointer_to_actor;

  gint block_count;

  guint update_idle_id;

  GHashTable *uuid_to_sw_items;

  ClutterActor *welcome_tile;

  GConfClient *gconf_client;
  gfloat ratio;
  guint ratio_notify_id;

  guint refresh_id;
};

static void
penge_everything_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_everything_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_everything_pane_dispose (GObject *object)
{
  PengeEverythingPanePrivate *priv = GET_PRIVATE (object);

  if (priv->refresh_id != 0)
  {
    g_source_remove (priv->refresh_id);
    priv->refresh_id = 0;
  }

  if (priv->pointer_to_actor)
  {
    g_hash_table_unref (priv->pointer_to_actor);
    priv->pointer_to_actor = NULL;
  }

  if (priv->uuid_to_sw_items)
  {
    g_hash_table_unref (priv->uuid_to_sw_items);
    priv->uuid_to_sw_items = NULL;
  }

  while (priv->views)
  {
    g_object_unref ((GObject *)priv->views->data);
    priv->views = g_list_delete_link (priv->views, priv->views);
  }

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }


  if (priv->recent_log)
    {
      if (priv->recent_monitor)
        /* remove the weak ref */
        zeitgeist_log_remove_monitor (priv->recent_log,
                                      priv->recent_monitor);

      g_object_unref (priv->recent_log);
      priv->recent_log = NULL;
    }

  if (priv->recent_monitor)
    {
      g_signal_handlers_disconnect_by_func (priv->recent_monitor,
          (GCallback)_zeitgeist_monitor_events_inserted_signal,
          PENGE_EVERYTHING_PANE(object));

      g_signal_handlers_disconnect_by_func (priv->recent_monitor,
          (GCallback)_zeitgeist_monitor_events_deleted_signal,
          PENGE_EVERYTHING_PANE(object));

      g_object_unref (priv->recent_monitor);
      priv->recent_monitor = NULL;
    }


  if (priv->ratio_notify_id)
  {
    gconf_client_notify_remove (priv->gconf_client,
                                priv->ratio_notify_id);
    priv->ratio_notify_id = 0;
  }

  if (priv->gconf_client)
  {
    g_object_unref (priv->gconf_client);
    priv->gconf_client = NULL;
  }

  G_OBJECT_CLASS (penge_everything_pane_parent_class)->dispose (object);
}

static void
penge_everything_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_everything_pane_parent_class)->finalize (object);
}

static void
penge_everything_pane_class_init (PengeEverythingPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeEverythingPanePrivate));

  object_class->get_property = penge_everything_pane_get_property;
  object_class->set_property = penge_everything_pane_set_property;
  object_class->dispose = penge_everything_pane_dispose;
  object_class->finalize = penge_everything_pane_finalize;
}

/* Sort funct for sorting recent files */
static gint
_recent_files_sort_func (ZeitgeistEvent *a,
                         ZeitgeistEvent *b)
{
  time_t time_a;
  time_t time_b;

  time_a = zeitgeist_event_get_timestamp (a);
  time_b = zeitgeist_event_get_timestamp (b);

  if (time_a > time_b)
  {
    return -1;
  } else if (time_a < time_b) {
    return 1;
  } else {
    return 0;
  }
}

/* Compare a SwItem with a ZeitgeistEvent */
static gint
_compare_item_and_event (SwItem           *item,
                         ZeitgeistEvent *event)
{
  time_t time_a;
  time_t time_b;

  /* Prefer info */
  if (item == NULL)
    return 1;

  /* Prefer item */
  if (event == NULL)
    return -1;

  time_a = item->date.tv_sec;
  time_b = zeitgeist_event_get_timestamp (event);

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
_people_tile_remove_clicked_cb (PengeInterestingTile *tile,
                                gpointer              userdata)
{
  PengeEverythingPane *pane = (PengeEverythingPane *)userdata;
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);
  SwClientService *service;
  SwItem *item;

  g_object_get (tile,
                "item", &item,
                NULL);

  service = sw_client_get_service (priv->client, item->service);

  sw_client_service_banishable_hide_item (service, item->uuid);

  g_object_unref (service);
}

static ClutterActor *
_add_from_sw_item (PengeEverythingPane *pane,
                   SwItem              *item)
{
  ClutterActor *actor;

  actor = g_object_new (PENGE_TYPE_PEOPLE_TILE,
                        "item", item,
                        NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (pane), actor);

  g_signal_connect (actor,
                    "remove-clicked",
                    (GCallback)_people_tile_remove_clicked_cb,
                    pane);

  return actor;
}

static void
_zeitgeist_log_delete_events_received (GObject *source_object,
                                       GAsyncResult *res,
                                       gpointer user_data)
{
  ZeitgeistLog *log = ZEITGEIST_LOG (source_object);
  GError *error = NULL;

  g_warning ("deleted events");

  g_return_if_fail (PENGE_IS_INTERESTING_TILE (user_data));

  zeitgeist_log_delete_events_finish (log, res, &error);
  if (error != NULL)
    {
      g_warning (G_STRLOC ": Unable to remove item: %s",
          error->message);
      g_clear_error (&error);
    }
}


static void
_recent_file_tile_remove_clicked_cb (PengeInterestingTile *tile,
                                     gpointer              userdata)
{
  PengeEverythingPane *pane = (PengeEverythingPane *)userdata;
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);
  ZeitgeistEvent *event;
  GArray *event_ids;
  guint32 id;

  g_object_get (tile,
                "zg-event", &event,
                NULL);


  event_ids = g_array_sized_new (FALSE, FALSE, sizeof(guint32), 1);
  id = zeitgeist_event_get_id (event);
  g_array_append_val (event_ids, id);

  zeitgeist_log_delete_events (priv->recent_log,
                               event_ids,
                               NULL,
                               _zeitgeist_log_delete_events_received,
                               tile);
  g_array_unref (event_ids);

}

static ClutterActor *
_add_from_recent_file_event (PengeEverythingPane *pane,
                             ZeitgeistEvent       *event,
                             const gchar         *thumbnail_path)
{
  ClutterActor *actor;

  actor = g_object_new (PENGE_TYPE_RECENT_FILE_TILE,
                        "zg-event", event,
                        "thumbnail-path", thumbnail_path,
                        NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (pane), actor);

  g_signal_connect (actor,
                    "remove-clicked",
                    (GCallback)_recent_file_tile_remove_clicked_cb,
                    pane);

  return actor;
}

static gint
_sw_item_sort_compare_func (SwItem *a,
                            SwItem *b)
{
  if (a->date.tv_sec> b->date.tv_sec)
  {
    return -1;
  } else if (a->date.tv_sec == b->date.tv_sec) {
    return 0;
  } else {
    return 1;
  }
}


static gint
_sw_item_weight (SwItem *item)
{
  if (!sw_item_has_key (item, "thumbnail") &&
      sw_item_has_key (item, "content"))
  {
    return 2;
  } else {
    return 1;
  }
}

GList *
_filter_out_unshowable_recent_items (PengeEverythingPane *pane,
                                     ZeitgeistResultSet  *set)
{
  GList *ret = NULL;
  guint i; /* loop index */

  /* probably an error (or an actual empty set, obv), we need to return empty
   * list anyway */
  if (set == NULL)
    return NULL;

  while (zeitgeist_result_set_has_next (set))
  {
    ZeitgeistEvent *event = NULL;
    gchar *thumbnail_path = NULL;

    /* We have to do this because we edit the list */
    event = zeitgeist_result_set_next (set);

    /* FIXME, so far this is the assumption, then we can use a better data
     * structure for managing events with multiple subjects */
    g_assert (zeitgeist_event_num_subjects (event) == 1);

    for (i = 0; i < zeitgeist_event_num_subjects (event); ++i)
      {
        ZeitgeistSubject *s;
        GFile *file;
        const gchar *uri = NULL;

        s = zeitgeist_event_get_subject (event, i);
        g_assert (s != NULL);
        uri = zeitgeist_subject_get_uri (s);
        g_assert (uri != NULL);

        /* Just a small debug help :)
        g_warning ("%s: subj(%d): i=%s m=%s",
            zeitgeist_subject_get_uri (s),
            i,
            zeitgeist_subject_get_interpretation (s),
            zeitgeist_subject_get_manifestation (s));
        */


        /* Current detault template look for local files only, if it's not local,
         * it's probably a template error, log it and move on */
        if (!g_str_has_prefix (uri, "file:"))
          {
            g_warning ("uri %s for recent event is not local", uri);
            continue;
          }

        file = g_file_new_for_uri (uri);

        /* if the file does not exist anymore we ignore it and move on */
        if (!g_file_query_exists (file, NULL))
          break; /* consider the next event */

        thumbnail_path = mpl_utils_get_thumbnail_path (uri);

        /* add to the return list only if it has a valid thumbnail */
        if (thumbnail_path && g_file_test (thumbnail_path, G_FILE_TEST_EXISTS))
          ret = g_list_prepend (ret, g_object_ref (event));

        g_free (thumbnail_path);
        g_object_unref (file);
      }
  }

  return ret;
}

static void
_zeitgeist_log_find_received (GObject *source_object,
                              GAsyncResult *res,
                              gpointer user_data)
{
  ZeitgeistLog *log = ZEITGEIST_LOG (source_object);
  PengeEverythingPane *pane = user_data;
  PengeEverythingPanePrivate *priv;
  GList *sw_items, *recent_file_items, *l;
  ZeitgeistResultSet *set = NULL;
  GList *old_actors = NULL;
  ClutterActor *actor;
  gboolean show_welcome_tile = TRUE;
  gint recent_files_count, sw_items_count;
  GError *error = NULL;

  g_return_if_fail (PENGE_IS_EVERYTHING_PANE (user_data));

  priv = GET_PRIVATE (pane);

  set = zeitgeist_log_find_events_finish (log, res, &error);
  if (error != NULL)
    {
      g_warning (G_STRLOC ": Error obtaining recent files: %s",
          error->message);
      g_clear_error (&error);
    }

  /* It actually moves the interesting events into a list */
  recent_file_items = _filter_out_unshowable_recent_items (pane, set);
  recent_file_items = g_list_sort (recent_file_items,
                                   (GCompareFunc)_recent_files_sort_func);

  /* Get Sw items */
  sw_items = g_hash_table_get_values (priv->uuid_to_sw_items);
  sw_items = g_list_sort (sw_items,
                          (GCompareFunc)_sw_item_sort_compare_func);

  recent_files_count = priv->block_count * priv->ratio;

  if (recent_files_count > g_list_length (recent_file_items))
    recent_files_count = g_list_length (recent_file_items);

  sw_items_count = priv->block_count - recent_files_count;

  old_actors = g_hash_table_get_values (priv->pointer_to_actor);

  if (sw_items != NULL || recent_file_items != NULL)
  {
    if (priv->welcome_tile)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                      priv->welcome_tile);
      priv->welcome_tile = NULL;
    }
  }

  while ((sw_items_count && sw_items) ||
         (recent_files_count && recent_file_items))
  {
    SwItem *sw_item = NULL;
    ZeitgeistEvent *recent_file_event = NULL;

    /* If no sw items -> force compare to favour recent file */
    if (sw_items_count && sw_items)
      sw_item = (SwItem *)sw_items->data;
    else
      sw_item = NULL;

    /* If no recent files -> force compare to favour sw stuff */
    if (recent_files_count && recent_file_items)
      recent_file_event = recent_file_items->data;
    else
      recent_file_event = NULL;

    if (_compare_item_and_event (sw_item, recent_file_event) < 1)
    {
      /* Sw item is newer */

      actor = g_hash_table_lookup (priv->pointer_to_actor,
                                   sw_item);
      if (!actor)
      {
        actor = _add_from_sw_item (pane, sw_item);
        g_hash_table_insert (priv->pointer_to_actor,
                             sw_item,
                             actor);

        /* Needed to remove from hash when we kill the actor */
        g_object_set_data (G_OBJECT (actor), "data-pointer", sw_item);
      }

      sw_items_count -= _sw_item_weight (sw_item);

      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   actor,
                                   "col-span", _sw_item_weight (sw_item),
                                   NULL);

      sw_items = g_list_remove (sw_items, sw_item);

      show_welcome_tile = FALSE;
    } else {
      /* Recent file item is newer */

      actor = g_hash_table_lookup (priv->pointer_to_actor,
                                   recent_file_event);

      if (!actor)
      {
        const gchar *uri = NULL;
        gchar *thumbnail_path = NULL;
        ZeitgeistSubject *subj;

        /* FIXME we assume there is only one subject */
        subj = zeitgeist_event_get_subject (recent_file_event, 0);
        uri = zeitgeist_subject_get_uri (subj);

        thumbnail_path = mpl_utils_get_thumbnail_path (uri);

        actor = _add_from_recent_file_event (pane,
                                            recent_file_event,
                                            thumbnail_path);
        g_free (thumbnail_path);
        g_hash_table_insert (priv->pointer_to_actor,
                             recent_file_event,
                             actor);

        /* Needed to remove from hash when we kill the actor */
        g_object_set_data (G_OBJECT (actor), "data-pointer", recent_file_event);

        show_welcome_tile = FALSE;
      }

      recent_files_count--;

      g_object_unref (recent_file_event);
      recent_file_items = g_list_remove (recent_file_items,
                                         recent_file_event);
    }

    clutter_container_lower_child (CLUTTER_CONTAINER (pane),
                                   actor,
                                   NULL);

    old_actors = g_list_remove (old_actors, actor);
  }

  for (l = old_actors; l; l = l->next)
  {
    gpointer p;
    p = g_object_get_data (G_OBJECT (l->data), "data-pointer");

    if (p)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                      CLUTTER_ACTOR (l->data));
      g_hash_table_remove (priv->pointer_to_actor, p);
    }
  }

  g_list_free (old_actors);

  if (show_welcome_tile && !priv->welcome_tile)
  {
    priv->welcome_tile = penge_welcome_tile_new ();
    clutter_container_add_actor (CLUTTER_CONTAINER (pane),
                                 priv->welcome_tile);
    clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                 priv->welcome_tile,
                                 "col-span", 3,
                                 NULL);
  }

  g_list_free (sw_items);

  for (l = recent_file_items; l; l = l->next)
  {
    ZeitgeistEvent *recent_file_event = l->data;
    g_object_unref (recent_file_event);
  }
  g_list_free (recent_file_items);

}

/* Zeitgeist templates are handled in a strange way within libzeitgeist:
 * the GDestroyFunc is overriden with NULL and each item is unreferenced after
 * having been evaluated (taking the ownership of a floating ref if needed).
 *
 * To avoid problems it's safer and easier to create a new template each time
 * it is used, given a default configuration */
static GPtrArray *
_default_template_factory (void)
{
  GPtrArray *ret = g_ptr_array_new ();

  g_ptr_array_add (ret, zeitgeist_event_new_full (
        NULL, /* interpretation */
        ZEITGEIST_ZG_USER_ACTIVITY, /* manifestation */
        "application:*", /* actor */
        zeitgeist_subject_new_full (
          "file:*", /* uri, just local files */
          NULL, /* interpretation */
          NULL, /* manifestation */
          NULL, /* mime-type */
          NULL, /* origin */
          NULL, /* text */
          NULL /* storage - auto-guess */
          ),
        NULL));

  return ret;
}


static void
penge_everything_pane_update (PengeEverythingPane *pane)
{
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  /* Get recent files and sort */
  zeitgeist_log_find_events (priv->recent_log,
                             zeitgeist_time_range_new_anytime (),
                             _default_template_factory (),
                             ZEITGEIST_STORAGE_STATE_ANY,
                             50, /* how many result should it return */
                             ZEITGEIST_RESULT_TYPE_MOST_RECENT_SUBJECTS,
                             NULL, /* cancellable */
                             _zeitgeist_log_find_received,
                             pane);
}

static gboolean
_update_idle_cb (gpointer userdata)
{
  PengeEverythingPane *pane = (PengeEverythingPane *)userdata;
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  penge_everything_pane_update (pane);

  priv->update_idle_id = 0;

  return FALSE;
}

static gboolean
_refresh_cb (PengeEverythingPane *pane)
{
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);
  gpointer key, value;
  PengePeopleTile *tile;
  GHashTableIter iter;

  g_hash_table_iter_init (&iter, priv->pointer_to_actor);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    if (PENGE_IS_PEOPLE_TILE (value)) {
      tile = (PengePeopleTile *) value;
      penge_people_tile_refresh (tile);
    }
  }

  return TRUE;
}

static void
penge_everything_pane_queue_update (PengeEverythingPane *pane)
{
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  if (priv->update_idle_id)
  {
    /* No need to update, we already have an update queued */
  } else {
    priv->update_idle_id = g_idle_add (_update_idle_cb, pane);
  }
}

static void
_view_items_added_cb (SwClientItemView *view,
                      GList            *items,
                      gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);
  GList *l;

  for (l = items; l; l = l->next)
  {
    SwItem *item = (SwItem *)l->data;
    g_debug (G_STRLOC ": Item added: %s", item->uuid);
    g_hash_table_insert (priv->uuid_to_sw_items,
                         g_strdup (item->uuid),
                         sw_item_ref (item));
  }

  penge_everything_pane_queue_update (pane);
}

static void
_view_items_removed_cb (SwClientItemView *view,
                        GList            *items,
                        gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  GList *l;

  for (l = items; l; l = l->next)
  {
    SwItem *item = (SwItem *)l->data;
    g_debug (G_STRLOC ": Item removed: %s", item->uuid);
    g_hash_table_remove (priv->uuid_to_sw_items,
                         item->uuid);
  }

  penge_everything_pane_queue_update (pane);
}

static void
_view_items_changed_cb (SwClientItemView *view,
                        GList            *items,
                        gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  GList *l;

  for (l = items; l; l = l->next)
  {
    SwItem *item = (SwItem *)l->data;
    ClutterActor *actor;

    g_debug (G_STRLOC ": Item changed: %s", item->uuid);

    /* Important to note that SwClientItemView reuses the SwItem so the
     * pointer is a valid piece of lookup
     */
    actor = g_hash_table_lookup (priv->pointer_to_actor,
                                 item);

    /* Can happily be NULL because we got a changed before we laid out the
     * actor
     */
    if (actor)
    {
      g_object_set (actor,
                    "item", item,
                    NULL);
    }
  }

  /* Do this because weights might have changed */
  penge_everything_pane_queue_update (pane);
}

static void
_client_open_view_cb (SwClientService  *service,
                      SwClientItemView *view,
                      gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  if (!view)
    return;

  priv->views = g_list_append (priv->views, view);

  g_signal_connect (view,
                    "items-added",
                    (GCallback)_view_items_added_cb,
                    userdata);
  g_signal_connect (view,
                    "items-removed",
                    (GCallback)_view_items_removed_cb,
                    userdata);
  g_signal_connect (view,
                    "items-changed",
                    (GCallback)_view_items_changed_cb,
                    userdata);

  sw_client_item_view_start (view);
}

static void
_client_get_services_cb (SwClient    *client,
                         GList *services,
                         gpointer     userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);
  GList *l;

  for (l = services; l; l = l->next)
  {
    SwClientService *service;

    service = sw_client_get_service (client, (const gchar *)l->data);

    sw_client_service_query_open_view (service,
                                       "feed",
                                       NULL,
                                       _client_open_view_cb,
                                       pane);

    g_object_unref (service);
  }
}

static void
_zeitgeist_monitor_events_inserted_signal (ZeitgeistMonitor *m,
                                           ZeitgeistTimeRange *time_range,
                                           GPtrArray *events,
                                           gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);

  g_warning ("event inserted, update");
  penge_everything_pane_queue_update (pane);
}


static void
_zeitgeist_monitor_events_deleted_signal (ZeitgeistMonitor *m,
                                          ZeitgeistTimeRange *time_range,
                                          GArray *ids,
                                          gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);

  g_warning ("event deleted, update");
  penge_everything_pane_queue_update (pane);
}

static void
_layout_count_changed_cb (PengeBlockContainer *pbc,
                          gint              count,
                          gpointer          userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);

  if (priv->block_count != count)
  {
    priv->block_count = count;
    penge_everything_pane_queue_update (pane);
  }
}

static void
_gconf_ratio_notify_cb (GConfClient *client,
                        guint        cnxn_id,
                        GConfEntry  *entry,
                        gpointer     userdata)
{
  PengeEverythingPane *pane = PENGE_EVERYTHING_PANE (userdata);
  PengeEverythingPanePrivate *priv = GET_PRIVATE (pane);
  GConfValue *value;

  value = gconf_entry_get_value (entry);
  if (!value)
    priv->ratio = 0.5;
  else
    priv->ratio = gconf_value_get_float (value);

  penge_everything_pane_queue_update (pane);
}

static void
penge_everything_pane_init (PengeEverythingPane *self)
{
  PengeEverythingPanePrivate *priv = GET_PRIVATE_REAL (self);
  gfloat tile_width, tile_height;
  GError *error = NULL;

  self->priv = priv;

  /* pointer to pointer */
  priv->pointer_to_actor = g_hash_table_new (NULL, NULL);

  /* For storing sw items */
  priv->uuid_to_sw_items = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  (GDestroyNotify)sw_item_unref);

  priv->client = sw_client_new ();
  sw_client_get_services (priv->client,
                          (SwClientGetServicesCallback)_client_get_services_cb,
                          self);

  priv->recent_log = g_object_new (ZEITGEIST_TYPE_LOG, NULL);
  priv->recent_monitor = zeitgeist_monitor_new (
                                            zeitgeist_time_range_new_anytime (),
                                            _default_template_factory ());

  g_signal_connect (priv->recent_monitor,
                    "events-inserted",
                    (GCallback)_zeitgeist_monitor_events_inserted_signal,
                    self);

  g_signal_connect (priv->recent_monitor,
                    "events-deleted",
                    (GCallback)_zeitgeist_monitor_events_deleted_signal,
                    self);

  /* the log holds a weak ref to the monitor */
  zeitgeist_log_install_monitor (priv->recent_log, priv->recent_monitor);

  penge_block_container_set_spacing (PENGE_BLOCK_CONTAINER (self), 5);

  priv->gconf_client = gconf_client_get_default ();

  tile_width = gconf_client_get_float (priv->gconf_client,
                                       DAWATI_MYZONE_MIN_TILE_WIDTH,
                                       NULL);

  /* Returns 0.0 if unset */
  if (tile_width == 0.0)
  {
    tile_width = TILE_WIDTH;
  }

  tile_height = gconf_client_get_float (priv->gconf_client,
                                        DAWATI_MYZONE_MIN_TILE_HEIGHT,
                                        NULL);

  if (tile_height == 0.0)
  {
    tile_height = TILE_HEIGHT;
  }

  penge_block_container_set_min_tile_size (PENGE_BLOCK_CONTAINER (self),
                                           tile_width,
                                           tile_height);

  g_signal_connect (self,
                    "count-changed",
                    (GCallback)_layout_count_changed_cb,
                    self);

  priv->ratio_notify_id =
    gconf_client_notify_add (priv->gconf_client,
                             DAWATI_MYZONE_RATIO,
                             _gconf_ratio_notify_cb,
                             self,
                             NULL,
                             &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error setting gconf key notification: %s",
               error->message);
    g_clear_error (&error);
  } else {
    gconf_client_notify (priv->gconf_client, DAWATI_MYZONE_RATIO);
  }

  priv->refresh_id = g_timeout_add_seconds (REFRESH_TIME, (GSourceFunc) _refresh_cb, self);
}


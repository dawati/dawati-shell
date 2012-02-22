/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
  * Copyright (c) 2012 Intel Corp.
  *
  * Author: Seif Lotfy <seif.lotfy@collabora.co.uk>
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

#include <string.h>

#include <zeitgeist.h>

#include "mnb-launcher-zg-utils.h"

typedef struct
{
  MnbLauncherZgUtilsGetAppsCB callback;
  gpointer data;
} MnbLauncherZgUtilsGetApps;

static ZeitgeistLog *zg_log = NULL;

static void ensure_log (void)
{
  if (zg_log == NULL)
    zg_log = g_object_new (ZEITGEIST_TYPE_LOG, NULL);

  g_assert (ZEITGEIST_IS_LOG (zg_log));
}

void
mnb_launcher_zg_utils_send_launch_event (const gchar *executable,
                                         const gchar *title)
{

  g_return_if_fail ((executable != NULL) && (*executable != '\0'));
  g_return_if_fail ((title != NULL) && (*title != '\0'));

  ensure_log ();

  char *exec = g_strconcat ("application://", executable, NULL);

 /* Zeitgeist event is handled in a strange way within libzeitgeist:
  * the GDestroyFunc is overriden with NULL and each item is unreferenced after
  * having been evaluated (taking the ownership of a floating ref if needed).*/

  ZeitgeistEvent *event = zeitgeist_event_new_full (
    ZEITGEIST_ZG_ACCESS_EVENT,
    ZEITGEIST_ZG_USER_ACTIVITY,
    "application://launcher.desktop",
    zeitgeist_subject_new_full (exec,
                                ZEITGEIST_NFO_SOFTWARE,      // interpretation
                                ZEITGEIST_NFO_SOFTWARE_ITEM, // manifestation
                                NULL,                        // mime-type
                                NULL,
                                title,
                                NULL),                       // storage - auto-guess
    NULL);

  zeitgeist_log_insert_events_no_reply (zg_log, event, NULL);
  g_free (exec);
}

static void
mnb_launcher_zg_utils_get_most_used_apps_cb (GObject      *object,
                                             GAsyncResult *res,
                                             gpointer      user_data)
{
  ZeitgeistLog *zg_log = (ZeitgeistLog *) object;
  ZeitgeistResultSet *events;
  MnbLauncherZgUtilsGetApps *cb_data;
  ZeitgeistEvent     *event;
  ZeitgeistSubject   *subject;

  GList *apps = NULL;
  GError *error = NULL;
  int prefix_len = strlen ("application://");
  int suffix_len = strlen (".desktop");

  events = zeitgeist_log_find_events_finish (zg_log, res, &error);
  cb_data = user_data;

  if (error == NULL)
    {
      while (zeitgeist_result_set_has_next (events))
        {
          event = zeitgeist_result_set_next (events);

          g_assert(zeitgeist_event_num_subjects(event) == 1);

          /* Since this application is the one pushing the events into Zeitgeist,
           * we can safely assume that the every event sent has only one subject */

          subject = zeitgeist_event_get_subject (event, 0);
          const char *app_uri = zeitgeist_subject_get_uri (subject);
          if (g_str_has_suffix (app_uri, ".desktop") &&
              g_str_has_prefix (app_uri, "application://"))
            {
              int len = strlen (app_uri);
              char *exec = g_strndup (app_uri + prefix_len,
                                      len - prefix_len - suffix_len);
              apps = g_list_append (apps, exec);
            }
        }
    }
  else
    {
      g_warning ("Error reading results: %s", error->message);
      g_error_free (error);
      return;
    }

  cb_data->callback (apps, cb_data->data);

  g_free (cb_data);
  g_object_unref (events);
  g_list_free_full (apps, g_free);

}

void
mnb_launcher_zg_utils_get_used_apps (MnbLauncherZgUtilsGetAppsCB  cb,
                                     gpointer                     user_data,
                                     const gchar                 *category)
{

  MnbLauncherZgUtilsGetApps *data;
  GPtrArray          *templates;
  int sorting;

  if (g_strcmp0 (category, "recent") == 0)
    sorting = 2;
  else if (g_strcmp0 (category, "most") == 0)
    sorting = 4;
  else
    return;

  ensure_log ();

 /* Zeitgeist templates are handled in a strange way within libzeitgeist:
  * the GDestroyFunc is overriden with NULL and each item is unreferenced after
  * having been evaluated (taking the ownership of a floating ref if needed).
  *
  * To avoid problems it's safer and easier to create a new template each time
  * it is used, given a default configuration */

  templates = g_ptr_array_new_full (0, g_object_unref);
  g_ptr_array_add (templates, zeitgeist_event_new_full (
                     ZEITGEIST_ZG_ACCESS_EVENT,
                     ZEITGEIST_ZG_USER_ACTIVITY,
                     "application://launcher.desktop",
                     zeitgeist_subject_new_full (
                       NULL,
                       ZEITGEIST_NFO_SOFTWARE, // interpretation
                       ZEITGEIST_NFO_SOFTWARE_ITEM, // manifestation
                       NULL, // mime-type
                       NULL,
                       NULL,
                       NULL // storage - auto-guess
                       ), NULL));

  data = g_new0 (MnbLauncherZgUtilsGetApps, 1);
  data->callback = cb;
  data->data = user_data;

  zeitgeist_log_find_events (zg_log,
                             zeitgeist_time_range_new_to_now (),
                             templates,
                             ZEITGEIST_STORAGE_STATE_ANY,
                             12,
                             sorting,
                             NULL,
                             mnb_launcher_zg_utils_get_most_used_apps_cb,
                             data);
}

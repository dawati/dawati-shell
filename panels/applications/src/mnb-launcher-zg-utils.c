/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2012 .Collabora
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

#include "mnb-launcher-zg-utils.h"
#include <zeitgeist.h>

ZeitgeistLog *log = NULL;

void
mnb_launcher_zg_utils_send_launch_event (const gchar* executable, const gchar* title) {

    char *exec = g_strconcat ("application://", executable, ".desktop", NULL);
    
    if (log == NULL) 
        log = zeitgeist_log_get_default ();

    ZeitgeistEvent *event = zeitgeist_event_new_full (
        ZEITGEIST_ZG_ACCESS_EVENT,
        ZEITGEIST_ZG_USER_ACTIVITY,
        "application://launcher.desktop",
        zeitgeist_subject_new_full (
                exec,
                ZEITGEIST_NFO_SOFTWARE, // interpretation
                ZEITGEIST_NFO_SOFTWARE_ITEM, // manifestation
                NULL, // mime-type
                NULL,
                title,
                NULL // storage - auto-guess
        ), 
        NULL);

    zeitgeist_log_insert_events_no_reply (log, event, NULL);
    g_free (exec);
}

static void 
mnb_launcher_zg_utils_get_most_used_apps_cb (ZeitgeistLog  *log,
                                      GAsyncResult *res,
                                      gpointer user_data) {
  ZeitgeistResultSet *events;
  GError             *error;
  gint                i;

  GList* apps = NULL;
  events = zeitgeist_log_find_events_finish (log, res, error);
  mnb_launcher_zg_utils_cb_struct *cb_data = (mnb_launcher_zg_utils_cb_struct*) user_data;

  if (error == NULL)
    {
      while (zeitgeist_result_set_has_next (events))
      {
        ZeitgeistEvent     *event;
        ZeitgeistSubject   *subject;
        
        event = zeitgeist_result_set_next (events);
        subject = zeitgeist_event_get_subject (event, 0);
        char* app_uri = zeitgeist_subject_get_uri (subject);
        int prefix_len = strlen("application://");
        int suffix_len = strlen(".desktop");
        int len = strlen(app_uri);
        char *exec = g_strndup (app_uri + prefix_len, len - prefix_len - suffix_len);
        exec = g_strstrip(exec);
        apps = g_list_append(apps, exec);
      }
    }
  else
    {
      g_warning ("Error reading results: %s", error->message);
      g_error_free (error);
      return;
    }

  cb_data->callback(apps, cb_data->data);
  g_object_unref(events);
  g_list_free(apps);
  g_free(cb_data);
}

void
mnb_launcher_zg_utils_get_most_used_apps (mnb_launcher_zg_utils_cb_struct *data) {

  if (log == NULL) 
    log = zeitgeist_log_get_default ();
    
  GPtrArray          *templates;

  templates = g_ptr_array_new ();
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
  
  zeitgeist_log_find_events (log,
                         zeitgeist_time_range_new_to_now (),
                         templates,
                         ZEITGEIST_STORAGE_STATE_ANY,
                         12,
                         4,
                         NULL,
                         (GAsyncReadyCallback)mnb_launcher_zg_utils_get_most_used_apps_cb,
                         data);
}

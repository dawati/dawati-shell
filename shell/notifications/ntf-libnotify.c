/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Authors: Tomas Frydrych <tf@linux.intel.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <string.h>
#include <glib/gi18n.h>

#include "dawati-netbook-notify-store.h"
#include "ntf-libnotify.h"
#include "../dawati-netbook.h"
#include "ntf-notification.h"
#include "ntf-tray.h"
#include "ntf-overlay.h"

#define DAWATI_KEY_PREFIX "dawati:"

static guint32 subsystem_id = 0;
static DawatiNetbookNotifyStore *store = NULL;

static void
ntf_libnotify_update (NtfNotification *ntf, Notification *details);

typedef struct
{
  NtfNotification *notification;
  gint   id;
  gchar *action;
  DawatiNetbookNotifyStore *store;
} ActionData;

static gint     n_notifiers;
static gboolean overlay_focused;

static void
free_action_data (gpointer action)
{
  ActionData *a = (ActionData*)action;

  g_free (a->action);

  g_slice_free (ActionData, action);
}

static void
ntf_libnotify_action_cb (ClutterActor *button, ActionData *data)
{
  dawati_netbook_notify_store_action (data->store,
                                      data->id,
                                      data->action);
}

static void
ntf_libnotify_update_modal (void)
{
  NtfTray      *tray = ntf_overlay_get_tray (TRUE);
  MetaPlugin *plugin = dawati_netbook_get_plugin_singleton ();

  if (ntf_tray_get_n_notifications (tray) > 0)
    {
      if (!overlay_focused)
        {
          MetaScreen   *screen = meta_plugin_get_screen (plugin);
          ClutterActor *stage = meta_get_stage_for_screen (screen);
          MxFocusManager *manager =
            mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));

          dawati_netbook_stash_window_focus (plugin, CurrentTime);
          mx_focus_manager_push_focus (manager, MX_FOCUSABLE (tray));
          overlay_focused = TRUE;
        }
    }
  else
    {
      if (overlay_focused)
        {
          dawati_netbook_unstash_window_focus (plugin, CurrentTime);
          overlay_focused = FALSE;
        }
    }
}

/*
 * Handler for the NtfNotify::closed signal
 */
static void
ntf_libnotify_ntf_closed_cb (NtfNotification *ntf, gpointer dummy)
{

  n_notifiers--;
  if (n_notifiers < 0)
    {
      g_warning ("Bug in notifier accounting, attempting to fix");
      n_notifiers = 0;
    }

  ntf_libnotify_update_modal ();
  dawati_netbook_notify_store_close (store,
                                     ntf_notification_get_id (ntf),
                                     ClosedDismissed);
}

static void
ntf_libnotify_notification_added_cb (DawatiNetbookNotifyStore *store,
                                     Notification             *notification,
                                     gpointer                  data)
{
  NtfTray         *tray;
  NtfNotification *ntf;
  NtfSource       *src = NULL;
  gint             pid = notification->pid;
  const gchar     *machine = "local";

  tray = ntf_overlay_get_tray (notification->is_urgent);

  ntf = ntf_tray_find_notification (tray, subsystem_id, notification->id);

  if (!ntf)
    {
      gchar *srcid = g_strdup_printf ("application-%d@%s", pid, machine);

      if (!(src = ntf_sources_find_for_id (srcid)))
        {
          if ((src = ntf_source_new_for_pid (machine, pid)))
            ntf_sources_add (src);
        }

      if (src)
        {
          gboolean no_dismiss = (notification->no_dismiss_button != 0);

          ntf = ntf_notification_new (src,
                                      subsystem_id,
                                      notification->id,
                                      no_dismiss);
        }


      if (ntf)
        {
          n_notifiers++;
          g_signal_connect_after (ntf, "closed",
                                  G_CALLBACK (ntf_libnotify_ntf_closed_cb),
                                  NULL);

          ntf_libnotify_update (ntf, notification);
          ntf_tray_add_notification (tray, ntf);
          ntf_libnotify_update_modal ();
        }

      g_free (srcid);
    }
  else
    ntf_libnotify_update (ntf, notification);
}

static void
ntf_libnotify_notification_closed_cb (DawatiNetbookNotifyStore *store,
                                      guint                     id,
                                      guint                     reason,
                                      gpointer                  data)
{
  NtfTray         *tray;
  NtfNotification *ntf;

  /*
   * Look first in the regular tray for this id, then the urgent one.
   */
  tray = ntf_overlay_get_tray (FALSE);

  if (!(ntf = ntf_tray_find_notification (tray, subsystem_id, id)))
    {
      tray = ntf_overlay_get_tray (TRUE);
      ntf  = ntf_tray_find_notification (tray, subsystem_id, id);
    }

  if (ntf && !ntf_notification_is_closed (ntf))
    {
      n_notifiers--;
      if (n_notifiers < 0)
        {
          g_warning ("Bug in notifier accounting, attempting to fix");
          n_notifiers = 0;
        }
      ntf_notification_close (ntf);
      ntf_libnotify_update_modal ();
    }
}

void
ntf_libnotify_init (void)
{
  n_notifiers = 0;
  overlay_focused = FALSE;

  store = dawati_netbook_notify_store_new ();

  subsystem_id = ntf_notification_get_subsystem_id ();

  g_signal_connect (store,
                    "notification-added",
                    G_CALLBACK (ntf_libnotify_notification_added_cb),
                    NULL);

  g_signal_connect (store,
                    "notification-closed",
                    G_CALLBACK (ntf_libnotify_notification_closed_cb),
                    NULL);
}

static void
ntf_libnotify_update (NtfNotification *ntf, Notification *details)
{
  ClutterActor *icon = NULL;

  g_return_if_fail (store && ntf && details);

  if (details->summary)
    ntf_notification_set_summary (ntf, details->summary);

  if (details->body)
    ntf_notification_set_body (ntf, details->body);

  if (details->icon_pixbuf)
    {
      GdkPixbuf *pixbuf = details->icon_pixbuf;

      icon = clutter_texture_new ();

      clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (icon),
                                         gdk_pixbuf_get_pixels (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf),
                                         gdk_pixbuf_get_width (pixbuf),
                                         gdk_pixbuf_get_height (pixbuf),
                                         gdk_pixbuf_get_rowstride (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf) ?
                                         4 : 3,
                                         0, NULL);
    }
  else if (details->icon_name)
    {
      GtkIconTheme *theme;
      GtkIconInfo  *info;
      gchar        *filename = NULL;

      theme = gtk_icon_theme_get_default ();
      info = gtk_icon_theme_lookup_icon (theme, details->icon_name, 24, 0);

      if (info)
        {
          filename = g_strdup (gtk_icon_info_get_filename (info));
          gtk_icon_info_free (info);
        }
      else
        {
          char *scheme;

          scheme = g_uri_parse_scheme (details->icon_name);
          if (!g_strcmp0 ("file", scheme))
            {
              filename = g_filename_from_uri (details->icon_name,
                                              NULL,
                                              NULL);
            }
          g_free (scheme);
        }
      if (filename)
        {
          icon = clutter_texture_new ();
          clutter_texture_set_from_file (CLUTTER_TEXTURE(icon),
                                         filename,
                                         NULL);
          g_free (filename);
        }
    }

  if (icon)
    clutter_actor_set_size (icon, 24.0, 24.0);
  ntf_notification_set_icon (ntf, icon);

  if (details->actions)
    {
      GList *action;
      gchar *key, *value;

      ntf_notification_remove_all_buttons (ntf);

      for (action = details->actions; action;)
        {
          /*
           * The action list length is
           * guaranteed to be % 2 and > 0
           */
          key = action->data;
          action = g_list_next (action);
          value = action->data;
          action = g_list_next (action);

          if (strcasecmp(key, "default") != 0)
            {
              ActionData   *data;
              ClutterActor *button;
              KeySym        keysym = 0;

              data = g_slice_new0 (ActionData);
              data->notification = ntf;
              data->action       = g_strdup (key);
              data->id           = details->id;
              data->store        = store;

              button = mx_button_new ();

              mx_button_set_label (MX_BUTTON (button), value);

              g_signal_connect_data (button, "clicked",
                                     G_CALLBACK (ntf_libnotify_action_cb),
                                     data,
                                     (GClosureNotify) free_action_data,
                                     0);

              /*
               * Handle the dawati key shortcut protocol
               */
              if (!strncmp (key, DAWATI_KEY_PREFIX, strlen (DAWATI_KEY_PREFIX)))
                {
                  const char *k = key + strlen (DAWATI_KEY_PREFIX);
                  const char *pfx = strstr (k, "XK_");
                  char       *name;

                  if (pfx)
                    {
                      if (k == pfx)
                        {
                          name = g_strdup (k + 3);
                        }
                      else
                        {
                          name = g_strdup (k);

                          name [pfx - k] = 0;
                          strcat (name, pfx + 3);
                        }

                      keysym = XStringToKeysym (name);

                      if (!keysym)
                        g_warning (G_STRLOC ": no keysym found for %s (%s)",
                                   key, name);

                      g_free (name);

                    }
                  else
                    {
                      g_warning (G_STRLOC ": invalid key %s", key);
                    }
                }

              ntf_notification_add_button (ntf, button, key, keysym);
              mx_stylable_set_style_class (MX_STYLABLE (button),
                                           "Primary");
            }
        }
    }

  ntf_notification_set_urgent (ntf, details->is_urgent);

  ntf_notification_set_timeout (ntf, details->timeout_ms);
}

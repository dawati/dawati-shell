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

#include <string.h>
#include <glib/gi18n.h>

#include <display.h>

#include "../meego-netbook.h"
#include "ntf-notification.h"
#include "ntf-tray.h"
#include "ntf-overlay.h"

static guint32 subsystem_id = 0;

static void ntf_wm_demands_attention_clear (MetaWindow *window);

static void
ntf_wm_activate_cb (ClutterActor *button, MetaWindow *window)
{
  meego_netbook_activate_window (window);
}

static void
ntf_wm_meta_window_demands_attention_cb (MetaWindow *mw,
                                         GParamSpec *spec,
                                         gpointer    data)
{
  gboolean  demands_attention = FALSE;
  gboolean  urgent            = FALSE;

  g_object_get (G_OBJECT (mw),
                "demands-attention", &demands_attention,
                "urgent", &urgent,
                NULL);

  if (!demands_attention && !urgent)
    ntf_wm_demands_attention_clear (mw);
}

static void
ntf_wm_ntf_closed_cb (NtfNotification *ntf, gpointer dummy)
{
  NtfSource  *src    = ntf_notification_get_source (ntf);
  MetaWindow *window = ntf_source_get_window (src);

  /*
   * The source ensures that when the window goes away, it clears the internally
   * stored pointer before closing any notifications that go with this.
   */
  if (window)
    g_signal_handlers_disconnect_by_func (window,
                                        ntf_wm_meta_window_demands_attention_cb,
                                        NULL);
}

static void
ntf_wm_update_notification (NtfNotification *ntf, MetaWindow *window)
{
  ClutterActor *src_icon;
  ClutterActor *button;
  NtfSource    *src;
  const gchar  *title;
  const gchar  *summary;
  const gchar  *body;

  g_return_if_fail (ntf && window);

  src = ntf_notification_get_source (ntf);

  if ((src_icon = ntf_source_get_icon (src)))
    {
      ClutterActor *icon = clutter_clone_new (src_icon) ;

      ntf_notification_set_icon (ntf, icon);
    }

  title = meta_window_get_title (window);

  if (title)
    summary = title;
  else
    summary = _("Unknown window");

  ntf_notification_set_summary (ntf, summary);

  body = _("is asking for your attention.");

  ntf_notification_set_body (ntf, body);

  ntf_notification_remove_all_buttons (ntf);

  button = mx_button_new ();

  mx_button_set_label (MX_BUTTON (button), _("Activate"));

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ntf_wm_activate_cb),
                    window);

  ntf_notification_add_button (ntf, button, 0);
}

static void
ntf_wm_handle_demands_attention (MetaWindow *window)
{
  NtfTray         *tray;
  NtfNotification *ntf;
  NtfSource       *src = NULL;
  gint             pid;
  const gchar     *machine;
  gint             id;

  g_return_if_fail (META_IS_WINDOW (window));

  id      = GPOINTER_TO_INT (window);
  pid     = meta_window_get_pid (window);
  machine = meta_window_get_client_machine (window);

  tray = ntf_overlay_get_tray (FALSE);

  ntf = ntf_tray_find_notification (tray, subsystem_id, id);

  if (!ntf)
    {
      gchar *srcid = g_strdup_printf ("application-%d@%s", pid, machine);

      g_debug ("creating new notification for source %s", srcid);

      if (!(src = ntf_sources_find_for_id (srcid)))
        {
          if ((src = ntf_source_new_for_pid (machine, pid)))
            ntf_sources_add (src);
        }

      if (src)
        ntf = ntf_notification_new (src, subsystem_id, id, FALSE);

      if (ntf)
        {
          g_signal_connect (ntf, "closed",
                            G_CALLBACK (ntf_wm_ntf_closed_cb),
                            NULL);

          ntf_wm_update_notification (ntf, window);
          ntf_tray_add_notification (tray, ntf);
        }

      g_free (srcid);
    }
  else
    {
      g_debug ("updating existing notification");
      ntf_wm_update_notification (ntf, window);
    }

  g_signal_connect (window, "notify::demands-attention",
                    G_CALLBACK (ntf_wm_meta_window_demands_attention_cb),
                    NULL);
  g_signal_connect (window, "notify::urgent",
                    G_CALLBACK (ntf_wm_meta_window_demands_attention_cb),
                    NULL);
}

static void
ntf_wm_demands_attention_clear (MetaWindow *window)
{
  NtfTray         *tray;
  NtfNotification *ntf;
  gint             id = GPOINTER_TO_INT (window);

  /*
   * Look first in the regular tray for this id, then the urgent one.
   */
  tray = ntf_overlay_get_tray (FALSE);

  if ((ntf = ntf_tray_find_notification (tray, subsystem_id, id)) &&
      !ntf_notification_is_closed (ntf))
    {
      ntf_notification_close (ntf);
    }
}

static void
ntf_wm_display_window_demands_attention_cb (MetaDisplay  *display,
                                            MetaWindow   *mw,
                                            MutterPlugin *plugin)
{
  MutterWindow       *mcw;
  MetaCompWindowType  type;

  mcw = (MutterWindow*)meta_window_get_compositor_private (mw);

  g_return_if_fail (mcw);

  /*
   * Only use notifications for normal windows and dialogues.
   */
  type = mutter_window_get_window_type (mcw);

  if (!(type == META_COMP_WINDOW_NORMAL       ||
        type == META_COMP_WINDOW_MODAL_DIALOG ||
        type == META_COMP_WINDOW_DIALOG))
    {
      return;
    }

  if (mw != meta_display_get_focus_window (display))
    ntf_wm_handle_demands_attention (mw);
}

static void
ntf_wm_display_focus_window_notify_cb (MetaDisplay  *display,
                                       GParamSpec   *spec,
                                       MutterPlugin *plugin)
{
  MetaWindow *mw = meta_display_get_focus_window (display);

  if (mw)
    ntf_wm_demands_attention_clear (mw);
}

void
ntf_wm_init (void)
{
  MutterPlugin *plugin  = meego_netbook_get_plugin_singleton ();
  MetaScreen   *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay  *display = meta_screen_get_display (screen);

  subsystem_id = ntf_notification_get_subsystem_id ();

  g_signal_connect (display,
                    "window-demands-attention",
                    G_CALLBACK (ntf_wm_display_window_demands_attention_cb),
                    plugin);
  g_signal_connect (display,
                    "window-marked-urgent",
                    G_CALLBACK (ntf_wm_display_window_demands_attention_cb),
                    plugin);

  g_signal_connect (display,
                    "notify::focus-window",
                    G_CALLBACK (ntf_wm_display_focus_window_notify_cb),
                    plugin);
}


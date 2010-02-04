/*
 * Copyright © 2009, 2010, Intel Corporation.
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
 *
 * Authors: Thomas Wood <thomas.wood@intel.com>
 */

#include <mx/mx.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include "sw-overview.h"
#include "sw-zone.h"

#include <clutter/x11/clutter-x11.h>
#include <clutter-gtk/clutter-gtk.h>
#include <clutter/glx/clutter-glx.h>

static void
active_window_changed (WnckScreen *screen,
                       WnckWindow *prev,
                       SwOverview *overview)
{
  WnckWindow *win = wnck_screen_get_active_window (screen);

  if (!win)
    return;

  sw_overview_set_focused_window (SW_OVERVIEW (overview),
                                  wnck_window_get_xid (win));
}

static void
active_workspace_changed (WnckScreen    *screen,
                          WnckWorkspace *prev,
                          SwOverview    *overview)
{
  WnckWorkspace *ws = wnck_screen_get_active_workspace (screen);
  sw_overview_set_focused_zone (SW_OVERVIEW (overview),
                                wnck_workspace_get_number (ws));
}

static void
window_workspace_changed (SwWindow   *window,
                          gint        new_workspace,
                          WnckWindow *win)
{
  WnckScreen *screen = wnck_screen_get_default ();
  WnckWorkspace *ws;

  ws = wnck_screen_get_workspace (screen, new_workspace -1);

  if (!ws)
    {
      wnck_screen_change_workspace_count (screen, new_workspace);

      ws = wnck_screen_get_workspace (screen, new_workspace -1);
    }


  wnck_window_move_to_workspace (win, ws);
}

static void
window_closed_cb (SwWindow   *window,
                  WnckWindow *win)
{
  wnck_window_close (win, clutter_x11_get_current_event_time ());
}

static void
window_clicked_cb (SwWindow   *window,
                   WnckWindow *win)
{
  WnckWorkspace *ws;

  /* activate the window */
  wnck_window_activate (win, clutter_x11_get_current_event_time ());

  /* move to the workspace */
  ws = wnck_window_get_workspace (win);
  wnck_workspace_activate (ws, clutter_x11_get_current_event_time ());
}

static void
window_opened (WnckScreen *screen,
               WnckWindow *window,
               SwOverview *overview)
{
  SwWindow *win = (SwWindow*) sw_window_new ();
  WnckWorkspace *ws;
  gulong xid;
  ClutterActor *thumbnail;

  ws = wnck_window_get_workspace (window);

  xid = wnck_window_get_xid (window);
  sw_window_set_xid (win, xid);

  thumbnail = clutter_glx_texture_pixmap_new_with_window (xid);

  clutter_texture_set_keep_aspect_ratio (CLUTTER_TEXTURE (thumbnail), TRUE);
  sw_window_set_thumbnail (win, thumbnail);
  sw_window_set_title (win, wnck_window_get_name (window));
  sw_window_set_icon (win, (ClutterTexture*)
                      gtk_clutter_texture_new_from_pixbuf (wnck_window_get_icon (window)));

  sw_overview_add_window (overview, win, wnck_workspace_get_number (ws));

  g_signal_connect (win, "workspace-changed",
                    G_CALLBACK (window_workspace_changed), window);
  g_signal_connect (win, "close",
                    G_CALLBACK (window_closed_cb), window);
  g_signal_connect (win, "clicked",
                    G_CALLBACK (window_clicked_cb), window);
}

static void
window_closed (WnckScreen *screen,
               WnckWindow *window,
               SwOverview *overview)
{
  sw_overview_remove_window (overview, wnck_window_get_xid (window));
}

int
main (int argc, char **argv)
{
  ClutterActor *stage, *overview;
  GList  *windows, *l, *workspaces;
  gint count;
  WnckWorkspace *active_ws;

  clutter_init (&argc, &argv);
  gtk_init (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           "style/zones-panel.css", NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 1024, 600);

  overview = sw_overview_new ();

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), overview);
  clutter_actor_set_size (overview, 1024, 600);

  WnckScreen *screen = wnck_screen_get_default ();
  wnck_screen_force_update (screen);


  windows = wnck_screen_get_windows (screen);
  workspaces = wnck_screen_get_workspaces (screen);

  /* count the number of workspaces and add to switcher */
  active_ws = wnck_screen_get_active_workspace (screen);
  count = 0;
  for (l = workspaces; l; l = l->next)
    {
      sw_overview_add_zone (SW_OVERVIEW (overview));
      count++;
    }


  for (l = windows; l; l = g_list_next (l))
    {
      WnckWorkspace *workspace;
      WnckWindow *win = l->data;

      workspace = wnck_window_get_workspace (win);

      if (!workspace)
        continue;

      window_opened (screen, win, SW_OVERVIEW (overview));
    }

  g_signal_connect (screen, "active-window-changed",
                    G_CALLBACK (active_window_changed), overview);


  g_signal_connect (screen, "active-workspace-changed",
                    G_CALLBACK (active_workspace_changed), overview);

  g_signal_connect (screen, "window-closed",
                    G_CALLBACK (window_closed), overview);

  g_signal_connect (screen, "window-opened",
                    G_CALLBACK (window_opened), overview);

  clutter_actor_show (stage);

  clutter_main ();

  return 0;
}

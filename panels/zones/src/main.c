#include <mx/mx.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include "sw-overview.h"
#include "sw-zone.h"

#include <clutter/x11/clutter-x11.h>
#include <clutter-gtk/clutter-gtk.h>
#include <clutter/glx/clutter-glx.h>

#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>

#include <glib/gi18n.h>

static MplPanelClient *client = NULL;

typedef struct
{
  ClutterActor   *stage;
  ClutterActor   *view;
  gint            width;
  gint            height;
  gboolean        connected;
} ZonePanelData;

static void
_client_set_size_cb (MplPanelClient *client,
                     guint           w,
                     guint           h,
                     ZonePanelData  *data)
{
  data->width = w;
  data->height = h;
}

static gboolean standalone = FALSE;

static GOptionEntry entries[] = {
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, "Do not embed into the mutter-moblin panel", NULL},
  { NULL }
};



static void
active_window_changed (WnckScreen    *screen,
                       WnckWindow    *prev,
                       ZonePanelData *data)
{
  WnckWindow *win = wnck_screen_get_active_window (screen);

  if (!data->view)
    return;

  if (!win)
    return;

  sw_overview_set_focused_window (SW_OVERVIEW (data->view),
                                  wnck_window_get_xid (win));
}

static void
active_workspace_changed (WnckScreen    *screen,
                          WnckWorkspace *prev,
                          ZonePanelData *data)
{
  WnckWorkspace *ws = wnck_screen_get_active_workspace (screen);

  if (!data->view)
    return;

  if (!ws)
    {
      g_warning ("Active workspace changed, but no active workspace set!");
      return;
    }

  sw_overview_set_focused_zone (SW_OVERVIEW (data->view),
                                wnck_workspace_get_number (ws));
}

static void
workspace_added_for_window (WnckScreen *screen,
                            WnckWorkspace *ws,
                            WnckWindow *win)
{
  wnck_window_move_to_workspace (win, ws);

  g_debug ("Window (%s) moved to workspace %d",
           wnck_window_get_name (win), wnck_workspace_get_number (ws));

  g_signal_handlers_disconnect_by_func (screen, workspace_added_for_window, win);
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
      g_debug ("Changing workspace count to %d...", new_workspace);
      g_signal_connect (screen, "workspace-created",
                        G_CALLBACK (workspace_added_for_window), win);

      wnck_screen_change_workspace_count (screen, new_workspace);
    }
  else
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

  /* close panel */
  mpl_panel_client_hide (client);

  /* activate the window */
  wnck_window_activate (win, clutter_x11_get_current_event_time ());

  /* move to the workspace */
  ws = wnck_window_get_workspace (win);
  wnck_workspace_activate (ws, clutter_x11_get_current_event_time ());
}

static void
window_opened (WnckScreen    *screen,
               WnckWindow    *window,
               ZonePanelData *data)
{
  SwWindow *win = (SwWindow*) sw_window_new ();
  WnckWorkspace *ws;
  gulong xid;
  ClutterActor *thumbnail;

  if (!data->view)
    return;

  ws = wnck_window_get_workspace (window);

  if (!ws)
  {
    g_debug ("Could not get workspace for window (%s)",
             wnck_window_get_name (window));
    return;
  }

  xid = wnck_window_get_xid (window);
  sw_window_set_xid (win, xid);

  thumbnail = clutter_glx_texture_pixmap_new_with_window (xid);
  clutter_texture_set_keep_aspect_ratio (CLUTTER_TEXTURE (thumbnail), TRUE);

  sw_window_set_thumbnail (win, thumbnail);
  sw_window_set_title (win, wnck_window_get_name (window));
  sw_window_set_icon (win, (ClutterTexture*)
                      gtk_clutter_texture_new_from_pixbuf (wnck_window_get_icon (window)));

  sw_overview_add_window (SW_OVERVIEW (data->view), win,
                          wnck_workspace_get_number (ws));

  g_signal_connect (win, "workspace-changed",
                    G_CALLBACK (window_workspace_changed), window);
  g_signal_connect (win, "close",
                    G_CALLBACK (window_closed_cb), window);
  g_signal_connect (win, "clicked",
                    G_CALLBACK (window_clicked_cb), window);
}

static void
window_closed (WnckScreen    *screen,
               WnckWindow    *window,
               ZonePanelData *data)
{
  if (data->view)
    sw_overview_remove_window (SW_OVERVIEW (data->view),
                               wnck_window_get_xid (window));
}

static void
setup (ZonePanelData *data)
{
  GList  *windows, *l;
  WnckScreen *screen;

  screen = wnck_screen_get_default ();
  wnck_screen_force_update (screen);

  windows = wnck_screen_get_windows (screen);

  /* create the overview */
  data->view = sw_overview_new (wnck_screen_get_workspace_count (screen));
  clutter_actor_set_size (data->view, data->width, data->height);

  /* add existing windows */
  for (l = windows; l; l = g_list_next (l))
    {
      WnckWorkspace *workspace;
      WnckWindow *win = l->data;

      workspace = wnck_window_get_workspace (win);

      if (!workspace)
        continue;

      window_opened (screen, win, data);
    }

  /* ensure the current workspace is marked */
  active_workspace_changed (screen, NULL, data);

  if (!data->connected)
    {
      data->connected = TRUE;

      g_signal_connect (screen, "active-window-changed",
                        G_CALLBACK (active_window_changed), data);


      g_signal_connect (screen, "active-workspace-changed",
                        G_CALLBACK (active_workspace_changed), data);

      g_signal_connect (screen, "window-closed",
                        G_CALLBACK (window_closed), data);

      g_signal_connect (screen, "window-opened",
                        G_CALLBACK (window_opened), data);
    }
}

static void
show (ZonePanelData *data)
{
  setup (data);
  clutter_actor_show_all (data->view);
  clutter_container_add_actor (CLUTTER_CONTAINER (data->stage), data->view);
}

static void
hide (ZonePanelData *data)
{
  clutter_actor_destroy (data->view);
  data->view = NULL;
}

int
main (int argc, char **argv)
{
  GOptionContext *context;
  GError *error = NULL;

  ZonePanelData *data;

  data = g_new0 (ZonePanelData, 1);

  context = g_option_context_new ("- mutter-moblin zones panel");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context,
                              clutter_get_option_group_without_init ());
  g_option_context_add_group (context, gtk_get_option_group (FALSE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical (G_STRLOC ": Error parsing option: %s", error->message);
    g_clear_error (&error);
  }

  g_option_context_free (context);

  MPL_PANEL_CLUTTER_INIT_WITH_GTK (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           STYLEDIR "/switcher.css", NULL);

  if (!standalone)
    {
      client = mpl_panel_clutter_new ("zones",
                                      _("zones"),
                                      NULL,
                                      "zones-button",
                                      TRUE);

      MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK (client);

      mpl_panel_client_set_height_request (client, 500);

      data->stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));

      g_signal_connect (client,
                        "set-size",
                        (GCallback)_client_set_size_cb,
                        data);

      mpl_panel_client_request_tooltip (client, "zones");

      g_signal_connect_swapped (data->stage, "hide", G_CALLBACK (hide), data);
      g_signal_connect_swapped (data->stage, "show", G_CALLBACK (show), data);
    }
  else
    {
      Window xwin;

      data->stage = clutter_stage_get_default ();
      clutter_actor_realize (data->stage);
      xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (data->stage));

      setup (data);

      MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK_FOR_XID (xwin);
      clutter_actor_set_size ((ClutterActor *) data->view, 1016, 500);
      clutter_actor_set_size (data->stage, 1016, 500);
      clutter_actor_show_all (data->stage);

      clutter_container_add_actor (CLUTTER_CONTAINER (data->stage),
                                   (ClutterActor *) data->view);
    }



  clutter_main ();


  g_free (data);

  return 0;
}

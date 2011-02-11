
#include <config.h>

#include <mx/mx.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include "sw-overview.h"
#include "sw-zone.h"

#include <clutter/x11/clutter-x11.h>
#include <clutter-gtk/clutter-gtk.h>
#include <clutter/glx/clutter-glx.h>

#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>

#include "penge-magic-texture.h"

#include <glib/gi18n.h>
#include <locale.h>

#include <gconf/gconf-client.h>

#define ZONES_PANEL_TOOLTIP _("zones")

static MplPanelClient *client = NULL;

typedef struct
{
  ClutterActor   *stage;
  ClutterActor   *overview;
  ClutterActor   *toplevel;
  gint            width;
  gint            height;
  gboolean        connected;
  ClutterActor   *background;
  GConfClient    *gconf_client;
  WnckScreen     *screen;
} ZonePanelData;

typedef struct
{
  WnckWindow *wnck_window;
  SwWindow *sw_window;
} WindowNotify;

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
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, "Do not embed into the mutter-meego panel", NULL},
  { NULL }
};


/* background loading */
#define BG_KEY_DIR "/desktop/gnome/background"
#define KEY_BG_FILENAME BG_KEY_DIR "/picture_filename"
#define THEMEDIR DATADIR "/mutter-meego/theme/"


static void
desktop_filename_changed_cb (GConfClient *client,
                             guint        cnxn_id,
                             GConfEntry  *entry,
                             gpointer     userdata)
{
  const gchar   *filename = NULL;
  GConfValue    *value;
  ZonePanelData *data = userdata;
  GError        *err = NULL;

  value = gconf_entry_get_value (entry);

  if (value)
    filename = gconf_value_get_string (value);

  if (!filename || !*filename)
    filename = THEMEDIR "/panel/background-tile.png";

  if (data->background)
    {
      clutter_actor_destroy (data->background);
      data->background = NULL;
    }

  if (!data->background)
    data->background = clutter_texture_new_from_file (filename, &err);
  else
    clutter_texture_set_from_file (CLUTTER_TEXTURE (data->background),
                                   filename, &err);


  if (err)
    {
      g_warning ("Could not load background image: %s", err->message);
      g_error_free (err);
      return;
    }

  clutter_texture_set_repeat (CLUTTER_TEXTURE (data->background), TRUE, TRUE);
  clutter_actor_set_size (data->background,
                          wnck_screen_get_width (data->screen),
                          wnck_screen_get_height (data->screen));
  clutter_container_add_actor (CLUTTER_CONTAINER (data->stage), data->background);
  clutter_actor_set_opacity (data->background, 0);
}

static void
desktop_background_init (ZonePanelData *data)
{
  GError *error = NULL;

  data->gconf_client = gconf_client_get_default ();


  gconf_client_add_dir (data->gconf_client,
                        BG_KEY_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        &error);

  if (error)
    {
      g_warning (G_STRLOC ": Error when adding directory for notification: %s",
                 error->message);
      g_clear_error (&error);
    }

  gconf_client_notify_add (data->gconf_client,
                           KEY_BG_FILENAME,
                           desktop_filename_changed_cb,
                           data,
                           NULL,
                           &error);

  if (error)
    {
      g_warning (G_STRLOC ": Error when adding key for notification: %s",
                 error->message);
      g_clear_error (&error);
    }

  /*
   * Read the background via our notify func
   */
  gconf_client_notify (data->gconf_client, KEY_BG_FILENAME);
}


static void
active_window_changed (WnckScreen    *screen,
                       WnckWindow    *prev,
                       ZonePanelData *data)
{
  WnckWindow *win = wnck_screen_get_active_window (screen);

  if (!data->overview)
    return;

  if (!win)
    return;

  sw_overview_set_focused_window (SW_OVERVIEW (data->overview),
                                  wnck_window_get_xid (win));
}

static void
active_workspace_changed (WnckScreen    *screen,
                          WnckWorkspace *prev,
                          ZonePanelData *data)
{
  WnckWorkspace *ws = wnck_screen_get_active_workspace (screen);

  if (!data->overview)
    return;

  if (!ws)
    {
      g_warning ("Active workspace changed, but no active workspace set!");
      return;
    }

  sw_overview_set_focused_zone (SW_OVERVIEW (data->overview),
                                wnck_workspace_get_number (ws));
}

static void
workspace_added_for_window (WnckScreen *screen,
                            WnckWorkspace *ws,
                            WnckWindow *win)
{
  wnck_window_move_to_workspace (win, ws);

  g_debug ("Workspace %d added for \"%s\"",
           wnck_workspace_get_number (ws) + 1, wnck_window_get_name (win));

  g_signal_handlers_disconnect_by_func (screen, workspace_added_for_window, win);
}

static void
window_workspace_changed (SwWindow   *window,
                          gint        new_workspace,
                          WnckWindow *win)
{
  WnckWorkspace *ws;
  WnckScreen *screen;

  screen = wnck_window_get_screen (win);

  ws = wnck_screen_get_workspace (screen, new_workspace -1);

  g_debug ("\"%s\" moved to workspace %d",
           wnck_window_get_name (win), new_workspace);

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


static void window_clicked_cb (SwWindow *window, WnckWindow *win);

static gboolean
activate_window (WindowNotify *notify)
{
  window_clicked_cb (notify->sw_window, notify->wnck_window);

  /* notify data automatically freed when the timeout is removed */
  return FALSE;
}

static void
remove_window_close_timer (gpointer *data)
{
  g_source_remove (GPOINTER_TO_INT (data));
}

static void
window_closed_cb (SwWindow   *window,
                  WnckWindow *win)
{
  WindowNotify *notify;
  guint tag;

  wnck_window_close (win, clutter_x11_get_current_event_time ());

  /* if this window hasn't closed in 150ms, activate it */

  notify = g_new (WindowNotify, 1);
  notify->sw_window = window;
  notify->wnck_window = win;
  tag = g_timeout_add_full (G_PRIORITY_DEFAULT, 150,
                            (GSourceFunc) activate_window, notify,
                            g_free);

  g_signal_connect_swapped (window, "destroy",
                            G_CALLBACK (remove_window_close_timer),
                            GINT_TO_POINTER (tag));
}

static void
switch_to (WnckWindow *win)
{
  WnckWorkspace *ws;

  ws = wnck_window_get_workspace (win);

  /* activate the window */
  wnck_window_activate (win, clutter_x11_get_current_event_time ());

  /* move to the workspace */
  wnck_workspace_activate (ws, clutter_x11_get_current_event_time ());
}

static void
hide_end (MplPanelClient *client,
          WnckWindow *window)
{
  switch_to (window);

  g_signal_handlers_disconnect_by_func (client, hide_end, window);
}

static void
window_clicked_cb (SwWindow   *window,
                   WnckWindow *win)
{

  /* close panel */
  if (client)
    {
      /* delayed switch */
      mpl_panel_client_hide (client);

      g_signal_connect (client, "hide-end",
                        G_CALLBACK (hide_end), win);

      return;
    }

  switch_to (win);
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
  ClutterActor *thumbnail_background;
  ClutterActor *icon;

  if (!data->overview)
    return;

  if (wnck_window_is_skip_pager (window))
    return;

  if (wnck_window_is_skip_tasklist (window))
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

  if (data->background)
    {
      thumbnail_background = g_object_new (PENGE_TYPE_MAGIC_TEXTURE,
                                           "cogl-texture",
                                           clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (data->background)),
                                           NULL);

      sw_window_set_background (win, thumbnail_background);
    }
  else
    g_debug ("No background found");

  thumbnail = clutter_glx_texture_pixmap_new_with_window (xid);
  clutter_x11_texture_pixmap_set_automatic (CLUTTER_X11_TEXTURE_PIXMAP (thumbnail),
                                            TRUE);
  clutter_texture_set_keep_aspect_ratio (CLUTTER_TEXTURE (thumbnail), TRUE);

  sw_window_set_thumbnail (win, thumbnail);
  sw_window_set_title (win, wnck_window_get_name (window));

  icon = gtk_clutter_texture_new ();
  gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (icon),
                                       wnck_window_get_icon (window),
                                       NULL);
  sw_window_set_icon (win, (ClutterTexture *)icon);

  sw_overview_add_window (SW_OVERVIEW (data->overview), win,
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
  if (data->overview)
    sw_overview_remove_window (SW_OVERVIEW (data->overview),
                               wnck_window_get_xid (window));
}

static void
setup (ZonePanelData *data)
{
  GList  *windows, *l;
  ClutterActor *box_layout, *title, *overview;

  box_layout = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box_layout),
                                 MX_ORIENTATION_VERTICAL);
  clutter_actor_set_name (box_layout, "zones-panel-toplevel");
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (box_layout), 6);

  data->toplevel = box_layout;
  clutter_actor_set_size (data->toplevel, data->width, data->height);


  title = mx_label_new_with_text (_("Zones"));
  clutter_actor_set_name (title, "zones-title-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (box_layout), title);


  wnck_screen_force_update (data->screen);

  windows = wnck_screen_get_windows (data->screen);

  /* create the overview */
  overview = sw_overview_new (wnck_screen_get_workspace_count (data->screen));
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (box_layout),
                                           overview, -1,
                                           "expand", TRUE,
                                           NULL);
  data->overview = overview;

  /* add existing windows */
  for (l = windows; l; l = g_list_next (l))
    {
      WnckWorkspace *workspace;
      WnckWindow *win = l->data;

      workspace = wnck_window_get_workspace (win);

      if (!workspace)
        continue;

      window_opened (data->screen, win, data);
    }

  /* ensure the current workspace is marked */
  active_workspace_changed (data->screen, NULL, data);

  if (!data->connected)
    {
      data->connected = TRUE;

      g_signal_connect (data->screen, "active-window-changed",
                        G_CALLBACK (active_window_changed), data);


      g_signal_connect (data->screen, "active-workspace-changed",
                        G_CALLBACK (active_workspace_changed), data);

      g_signal_connect (data->screen, "window-closed",
                        G_CALLBACK (window_closed), data);

      g_signal_connect (data->screen, "window-opened",
                        G_CALLBACK (window_opened), data);
    }
}

static void
show (ZonePanelData *data)
{
  setup (data);
  clutter_actor_show_all (data->toplevel);
  clutter_container_add_actor (CLUTTER_CONTAINER (data->stage), data->toplevel);
}

static void
hide (ZonePanelData *data)
{
  clutter_actor_destroy (data->toplevel);
  data->toplevel = NULL;
  data->overview = NULL;
}

static void
key_press (ClutterActor  *actor,
           ClutterEvent  *event,
           ZonePanelData *data)
{
  if (event->key.keyval != 32)
    return;

  hide (data);
  show (data);
}

int
main (int argc, char **argv)
{
  GOptionContext *context;
  GError *error = NULL;

  ZonePanelData *data;


  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);


  data = g_new0 (ZonePanelData, 1);

  context = g_option_context_new ("- mutter-meego zones panel");
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

  mpl_panel_clutter_init_with_gtk (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           STYLEDIR "/switcher.css", NULL);

  data->screen = wnck_screen_get_default ();
  wnck_screen_force_update (data->screen);


  if (!standalone)
    {
      client = mpl_panel_clutter_new ("zones",
                                      ZONES_PANEL_TOOLTIP,
                                      NULL,
                                      "zones-button",
                                      TRUE);

      mpl_panel_clutter_setup_events_with_gtk (MPL_PANEL_CLUTTER (client));

      mpl_panel_client_set_height_request (client, 600);

      data->stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));

      desktop_background_init (data);

      g_signal_connect (client,
                        "set-size",
                        (GCallback)_client_set_size_cb,
                        data);

      mpl_panel_client_request_tooltip (client, ZONES_PANEL_TOOLTIP);

      g_signal_connect_swapped (data->stage, "hide", G_CALLBACK (hide), data);
      g_signal_connect_swapped (data->stage, "show", G_CALLBACK (show), data);
    }
  else
    {
      Window xwin;

      data->stage = clutter_stage_get_default ();
      clutter_actor_realize (data->stage);
      xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (data->stage));

      desktop_background_init (data);

      data->width = 1016;
      data->height = 500;

      setup (data);

      mpl_panel_clutter_setup_events_with_gtk_for_xid (xwin);
      clutter_actor_set_size (data->stage, 1016, 500);
      clutter_actor_show_all (data->stage);

      clutter_container_add_actor (CLUTTER_CONTAINER (data->stage),
                                   (ClutterActor *) data->toplevel);

      g_signal_connect (data->stage, "key-release-event",
                        G_CALLBACK (key_press), data);
    }

  /* enable key focus support */
  mx_focus_manager_get_for_stage (CLUTTER_STAGE (data->stage));


  clutter_actor_lower_bottom (data->background);

  clutter_main ();


  g_free (data);

  return 0;
}


#include <config.h>

#include <mx/mx.h>
#define COGL_ENABLE_EXPERIMENTAL_API
#include <cogl/cogl-texture-pixmap-x11.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <clutter/x11/clutter-x11.h>
#include <clutter-gtk/clutter-gtk.h>
#include <clutter/glx/clutter-glx.h>

#include <dawati-panel/mpl-panel-clutter.h>
#include <dawati-panel/mpl-panel-common.h>

#include <glib/gi18n.h>
#include <locale.h>

#define ZONES_PANEL_TOOLTIP _("zones")

static MplPanelClient *client = NULL;

typedef struct
{
  ClutterActor   *stage;
  ClutterActor   *overview;
  ClutterActor   *toplevel;
  ClutterActor   *grid;
  gint            width;
  gint            height;
  gboolean        connected;
  ClutterActor   *background;
  WnckScreen     *screen;
} ZonePanelData;

typedef struct
{
  WnckWindow *wnck_window;
} WindowNotify;


static void update_toolbar_icon (WnckScreen    *screen,
                                 WnckWorkspace *space,
                                 ZonePanelData *data);

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
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone,
    "Do not embed into the mutter-dawati panel", NULL},
  { NULL }
};

static void
activate_workspace (ZonePanelData *data,
                    WnckWorkspace *ws)
{
  if (client)
    mpl_panel_client_hide (client);

  wnck_workspace_activate (ws, clutter_x11_get_current_event_time ());
}

static void
update_toolbar_icon (WnckScreen    *screen,
                     WnckWorkspace *space,
                     ZonePanelData *data)
{
  gint n_zones;
  GList *windows;

  n_zones = wnck_screen_get_workspace_count (screen);

  if (!client)
    return;

  if (n_zones == 1)
    {
      gint n_windows = 0;

      for (windows = wnck_screen_get_windows (screen);
           windows != NULL;
           windows = g_list_next (windows))
        {
          WnckWindow *window = windows->data;

          if (wnck_window_is_skip_pager (window))
            continue;

          if (wnck_window_is_skip_tasklist (window))
            continue;

          n_windows++;
        }

      if (n_windows == 0)
        n_zones = 0;
    }

  g_debug ("Number of zones: %d", n_zones);

  switch (n_zones)
    {
    case 1:
    case 2:
      mpl_panel_client_request_button_style (client, "zones-button-1");
      break;

    case 3:
    case 4:
      mpl_panel_client_request_button_style (client, "zones-button-2");
      break;

    case 5:
    case 6:
      mpl_panel_client_request_button_style (client, "zones-button-3");
      break;

    case 7:
    case 8:
      mpl_panel_client_request_button_style (client, "zones-button-4");
      break;

    default:
      mpl_panel_client_request_button_style (client, "zones-button");
      break;
    }
}

static gboolean
app_tile_release_event (ClutterActor *actor,
                        ClutterEvent *event,
                        ZonePanelData *data)
{
  WnckWindow *window;

  window = g_object_get_data (G_OBJECT (actor), "wnck-window");

  activate_workspace (data, wnck_window_get_workspace (window));

  return FALSE;
}

static void
close_workspace_btn_clicked (MxButton      *button,
                             WnckWorkspace *workspace)
{
  WnckScreen *screen;
  GList *windows;
  ClutterActor *table;

  screen = wnck_workspace_get_screen (workspace);

  windows = wnck_screen_get_windows (screen);

  while (windows)
    {
      if (wnck_window_get_workspace (windows->data) == workspace)
        {
          wnck_window_close (windows->data,
                             clutter_x11_get_current_event_time ());
        }

      windows = g_list_next (windows);
    }

  table = clutter_actor_get_parent (CLUTTER_ACTOR (button));
  g_assert (MX_IS_TABLE (table));
  clutter_actor_destroy (CLUTTER_ACTOR (table));
}

static ClutterActor *
sw_create_app_tile (ZonePanelData   *data,
                    WnckWindow      *window,
                    WnckApplication *application)
{
  ClutterActor *tile, *frame, *label, *icon, *button, *thumbnail, *shadow;

  /* tile */
  tile = mx_table_new ();
  g_object_set_data (G_OBJECT (tile), "wnck-window", window);
  clutter_actor_set_reactive (tile, TRUE);
  mx_stylable_set_style_class (MX_STYLABLE (tile), "switcherTile");
  clutter_actor_set_size (tile, 266, 212);
  g_signal_connect (tile, "button-release-event",
                    G_CALLBACK (app_tile_release_event), data);

  /* icon */
  icon = gtk_clutter_texture_new ();
  gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (icon),
                                       wnck_window_get_icon (window),
                                       NULL);
  mx_table_add_actor (MX_TABLE (tile), icon, 0, 0);
  mx_table_child_set_y_expand (MX_TABLE (tile), icon, FALSE);
  mx_table_child_set_x_expand (MX_TABLE (tile), icon, FALSE);
  mx_table_child_set_y_fill (MX_TABLE (tile), icon, FALSE);
  mx_table_child_set_row_span (MX_TABLE (tile), icon, 2);
  clutter_actor_set_size (icon, 32, 32);

  /* title */
  label = mx_label_new_with_text (wnck_application_get_name (application));
  mx_stylable_set_style_class (MX_STYLABLE (label), "appTitle");
  mx_table_add_actor (MX_TABLE (tile), label, 0, 1);
  mx_table_child_set_y_expand (MX_TABLE (tile), label, FALSE);

  /* subtitle */
  label = mx_label_new_with_text (wnck_window_get_name (window));
  mx_stylable_set_style_class (MX_STYLABLE (label), "appSubTitle");
  mx_table_add_actor (MX_TABLE (tile), label, 1, 1);
  mx_table_child_set_y_expand (MX_TABLE (tile), label, FALSE);

  /* close button */
  button = mx_button_new ();
  mx_stylable_set_style_class (MX_STYLABLE (button), "appCloseButton");
  mx_table_add_actor (MX_TABLE (tile), button, 0, 2);
  mx_table_child_set_y_fill (MX_TABLE (tile), button, FALSE);
  mx_table_child_set_x_fill (MX_TABLE (tile), button, FALSE);
  mx_table_child_set_y_expand (MX_TABLE (tile), button, FALSE);
  mx_table_child_set_x_expand (MX_TABLE (tile), button, FALSE);
  clutter_actor_set_size (button, 22, 21);
  g_signal_connect (button, "clicked", G_CALLBACK (close_workspace_btn_clicked),
                            wnck_window_get_workspace (window));

  /* frame */
  frame = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (frame), "appBackground");
  mx_table_add_actor (MX_TABLE (tile), frame, 2, 0);
  mx_table_child_set_column_span (MX_TABLE (tile), frame, 3);
  mx_table_child_set_x_expand (MX_TABLE (tile), frame, FALSE);

  /* shadow */
  shadow = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (shadow), "appShadow");
  mx_bin_set_child (MX_BIN (frame), shadow);
  mx_bin_set_fill (MX_BIN (frame), FALSE, FALSE);

  /* application thumbnail */
  thumbnail = clutter_glx_texture_pixmap_new_with_window (wnck_window_get_xid (window));
  clutter_x11_texture_pixmap_set_automatic (CLUTTER_X11_TEXTURE_PIXMAP (thumbnail),
                                            TRUE);
  clutter_texture_set_keep_aspect_ratio (CLUTTER_TEXTURE (thumbnail), TRUE);
  mx_bin_set_child (MX_BIN (shadow), thumbnail);

  return tile;
}

static void
setup (ZonePanelData *data)
{
  ClutterScript *script;
  GError *error = NULL;
  GList *workspaces, *l, *windows, *list;

  /* load custom style */
  mx_style_load_from_file (mx_style_get_default (),
                           DAWATI_STYLE_DIR "/switcher/switcher.css", &error);

  if (error)
    {
      g_critical ("Could not load Switcher styling: %s", error->message);
      g_clear_error (&error);
      return;
    }

  /* load user interface */
  script = clutter_script_new ();
  clutter_script_load_from_file (script, PKGDATADIR "/switcher.json", &error);
  if (error)
    {
      g_critical ("Could not load user interface: %s", error->message);
      g_clear_error (&error);
      return;
    }

  data->toplevel = (ClutterActor*) clutter_script_get_object (script,
                                                              "toplevel");
  clutter_actor_set_size (data->toplevel, data->width, data->height);

  data->grid = (ClutterActor*) clutter_script_get_object (script, "grid");

  wnck_screen_force_update (data->screen);

  workspaces = wnck_screen_get_workspaces (data->screen);
  windows = wnck_screen_get_windows_stacked (data->screen);
  for (l = workspaces; l; l = g_list_next (l))
    {
      WnckWindow *window = NULL;
      WnckWorkspace *workspace = l->data;
      WnckApplication *application;
      ClutterActor *tile;

      /* find the appropriate window */
      for (list = windows; list; list = g_list_next (list))
        {
          if (wnck_window_is_skip_pager (list->data)
              || wnck_window_is_skip_tasklist (list->data))
            continue;

          if (wnck_window_get_workspace (list->data) == workspace)
            window = list->data;
        }

      /* skip workspaces with no windows */
      if (!window)
        continue;

      application = wnck_window_get_application (window);

      tile = sw_create_app_tile (data, window, application);


      clutter_container_add_actor (CLUTTER_CONTAINER (data->grid), tile);
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

  context = g_option_context_new ("- mutter-dawati zones panel");
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
                           THEMEDIR "/switcher.css", NULL);

  data->screen = wnck_screen_get_default ();
  wnck_screen_force_update (data->screen);

  g_signal_connect (data->screen, "workspace-created",
                    G_CALLBACK (update_toolbar_icon), data);

  g_signal_connect (data->screen, "workspace-destroyed",
                    G_CALLBACK (update_toolbar_icon), data);

  if (!standalone)
    {
      client = mpl_panel_clutter_new ("zones",
                                      ZONES_PANEL_TOOLTIP,
                                      NULL,
                                      "zones-button",
                                      TRUE);

      update_toolbar_icon (data->screen, NULL, data);

      mpl_panel_clutter_setup_events_with_gtk (MPL_PANEL_CLUTTER (client));

      mpl_panel_client_set_height_request (client, 600);

      data->stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));

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

      data->width = 1024;
      data->height = 600;

      setup (data);

      mpl_panel_clutter_setup_events_with_gtk_for_xid (xwin);
      clutter_actor_set_size (data->stage, data->width, data->height);
      clutter_actor_show_all (data->stage);

      clutter_container_add_actor (CLUTTER_CONTAINER (data->stage),
                                   (ClutterActor *) data->toplevel);
    }

  /* enable key focus support */
  mx_focus_manager_get_for_stage (CLUTTER_STAGE (data->stage));


  if (data->background)
    clutter_actor_lower_bottom (data->background);

  clutter_main ();


  g_free (data);

  return 0;
}

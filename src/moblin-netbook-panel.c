/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *         Thomas Wood <thomas@linux.intel.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "moblin-netbook.h"
#include "moblin-netbook-panel.h"
#include "moblin-netbook-launcher.h"
#include "moblin-netbook-status.h"
#include "moblin-netbook-netpanel.h"
#include "moblin-netbook-pasteboard.h"
#include "moblin-netbook-people.h"

#include "penge/penge-grid-view.h"

#include <glib/gi18n.h>

#ifdef USE_AHOGHILL
#include "ahoghill/ahoghill-grid-view.h"
#endif

#include "mnb-switcher.h"
#include "mnb-panel-button.h"

#define BUTTON_WIDTH 66
#define BUTTON_HEIGHT 55
#define BUTTON_SPACING 10

#define TRAY_PADDING   3
#define TRAY_BUTTON_HEIGHT 55
#define TRAY_BUTTON_WIDTH 44

/* IF YOU EVER CHANGE THIS VALUE YOU NEED TO UPDATE CARRICK / DALSTON ETC */
#define PANEL_X_PADDING 4

struct button_data
{
  MutterPlugin *plugin;
  MnbkControl   control;
};

static void toggle_buttons_cb (NbtkButton *button, gpointer data);

/*
 * The slide-out top panel.
 */
static void
on_panel_back_effect_complete (ClutterTimeline *timeline, gpointer data)
{
  MutterPlugin               *plugin = data;
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gint i;

  priv->panel_back_in_progress = FALSE;

  /*
   * Hide the panel when not visible, and then any components with tooltips;
   * this ensures that also the tooltips get hidden.
   */
  clutter_actor_hide (priv->panel);

  for (i = 0; i < G_N_ELEMENTS (priv->panel_buttons); i++)
    {
      clutter_actor_hide (priv->panel_buttons[i]);
    }

  if (!priv->workspace_chooser)
    {
      if (priv->panel_input_region)
        {
          moblin_netbook_input_region_remove (plugin,
                                              priv->panel_input_region);
          priv->panel_input_region = NULL; /* XXX leaky? */
        }
      disable_stage (plugin, CurrentTime);
    }
}

struct panel_out_data
{
  MutterPlugin *plugin;
  MnbkControl   control;
};

static void
on_panel_out_effect_complete (ClutterTimeline *timeline, gpointer data)
{
  struct panel_out_data      *panel_data = data;
  MutterPlugin               *plugin = panel_data->plugin;
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  ClutterActor               *control_actor = NULL;
  struct button_data          button_data;
  int i;

  switch (panel_data->control)
    {
    case MNBK_CONTROL_MZONE:
      control_actor = priv->mzone_grid;
      break;

    case MNBK_CONTROL_SPACES:
      control_actor = priv->switcher;
      break;

    case MNBK_CONTROL_APPLICATIONS:
      control_actor = priv->launcher;
      break;

    case MNBK_CONTROL_MEDIA:
#ifdef USE_AHOGHILL
      control_actor = priv->media_drop_down;
      break;
#endif

    case MNBK_CONTROL_INTERNET:
#ifdef WITH_NETPANEL
      control_actor = priv->net_grid;
      break;
#endif

#ifdef WITH_PEOPLE
    case MNBK_CONTROL_PEOPLE:
      control_actor = priv->people;
      break;
#endif

    case MNBK_CONTROL_STATUS:
    case MNBK_CONTROL_PASTEBOARD:
      g_warning ("Control %d not handled (%s:%d)\n",
                 panel_data->control, __FILE__, __LINE__);
    default:
    case   MNBK_CONTROL_UNKNOWN:
      control_actor = NULL;
    }

  priv->panel_out_in_progress = FALSE;

  /* make sure no buttons are 'active' */
  button_data.plugin = plugin;
  button_data.control = MNBK_CONTROL_UNKNOWN;
  toggle_buttons_cb (NULL, &button_data);

  if (control_actor != priv->mzone_grid &&
      CLUTTER_ACTOR_IS_VISIBLE (priv->mzone_grid))
    {
      clutter_actor_hide (priv->mzone_grid);
    }

  if (control_actor != priv->switcher &&
      CLUTTER_ACTOR_IS_VISIBLE (priv->switcher))
    {
      clutter_actor_hide (priv->switcher);
    }

  if (control_actor != priv->launcher &&
      CLUTTER_ACTOR_IS_VISIBLE (priv->launcher))
    {
      clutter_actor_hide (priv->launcher);
    }

#ifdef WITH_NETPANEL
  if (control_actor != priv->net_grid &&
      CLUTTER_ACTOR_IS_VISIBLE (priv->net_grid))
    {
      clutter_actor_hide (priv->net_grid);
    }
#endif

#ifdef WITH_PEOPLE
  if (control_actor != priv->people &&
      CLUTTER_ACTOR_IS_VISIBLE (priv->people))
    {
      clutter_actor_hide (priv->people);
    }
#endif


  /* enable events for the buttons while the panel after the panel has stopped
   * moving
   */
  for (i = 0; i < G_N_ELEMENTS (priv->panel_buttons); i++)
    {
      clutter_actor_set_reactive (priv->panel_buttons[i], TRUE);
    }

  if (control_actor && !CLUTTER_ACTOR_IS_VISIBLE (control_actor))
    {
      NbtkButton *button =
        NBTK_BUTTON (priv->panel_buttons[(guint)panel_data->control-1]);

      nbtk_button_set_checked (button, TRUE);

      /*
       * Must reset the y in case a previous animation ended prematurely
       * and the y is not set correctly; see bug 900.
       */
      clutter_actor_set_y (control_actor, PANEL_HEIGHT);
      clutter_actor_show (control_actor);
    }

  /* basically make the whole stage go to clutter.. */
  priv->panel_input_region 
                = moblin_netbook_input_region_push (plugin,
                                                    0,
                                                    0,
                                                    priv->screen_width,
                                                    priv->screen_height,
                                                    FALSE);

  enable_stage (plugin, CurrentTime);

  g_free (data);
}

static void
show_panel_maybe_control (MutterPlugin *plugin,
                          gboolean      from_keyboard,
                          MnbkControl   control)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gint i;
  struct panel_out_data *panel_data = g_new0 (struct panel_out_data, 1);
  ClutterAnimation *animation;

  priv->panel_out_in_progress  = TRUE;

  panel_data->plugin = plugin;
  panel_data->control = control;

  for (i = 0; i < G_N_ELEMENTS (priv->panel_buttons); i++)
    {
      clutter_actor_show (priv->panel_buttons[i]);
      clutter_actor_set_reactive (priv->panel_buttons[i], FALSE);
    }

  if (!CLUTTER_ACTOR_IS_VISIBLE (priv->panel))
    {
      clutter_actor_show (priv->panel);

      animation = clutter_actor_animate (priv->panel,
                                         CLUTTER_EASE_IN_SINE,
                                         /* PANEL_SLIDE_TIMEOUT */ 150,
                                         "y", 0,
                                         NULL);

      g_signal_connect (clutter_animation_get_timeline (animation),
                        "completed",
                        G_CALLBACK (on_panel_out_effect_complete),
                        panel_data);
    }
  else
    on_panel_out_effect_complete (NULL, panel_data);

  if (from_keyboard)
    priv->panel_wait_for_pointer = TRUE;
}

void
show_panel (MutterPlugin *plugin, gboolean from_keyboard)
{
  show_panel_maybe_control (plugin, from_keyboard, MNBK_CONTROL_UNKNOWN);
}

void
show_panel_and_control (MutterPlugin *plugin, MnbkControl control)
{
  show_panel_maybe_control (plugin, FALSE, control);
}

/*
 * Returns TRUE if panel was hidden, FALSE otherwise.
 */
gboolean
hide_panel (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gint                        x;
  guint                       h;
  struct button_data button_data;
  ClutterAnimation *animation;

  if (priv->panel_wait_for_pointer)
    {
      g_debug ("Cannot hide panel -- waiting for pointer\n");
      return FALSE;
    }

  /*
   * Refuse to hide the panel if a config window is up.
   */
  if (shell_tray_manager_config_windows_showing (priv->tray_manager))
    {
      g_debug ("Cannot hide panel while config window is visible\n");
      return FALSE;
    }

  x = clutter_actor_get_x (priv->panel);
  h = clutter_actor_get_height (priv->panel_shadow);

  priv->panel_back_in_progress  = TRUE;

  animation = clutter_actor_animate (priv->panel,
                                     CLUTTER_EASE_IN_SINE,
                                     /* PANEL_SLIDE_TIMEOUT */ 150,
                                     "y", -h,
                                     NULL);

  g_signal_connect (clutter_animation_get_timeline (animation),
                    "completed",
                    G_CALLBACK (on_panel_back_effect_complete),
                    plugin);

  /* make sure no buttons are 'active' */
  button_data.plugin = plugin;
  button_data.control = MNBK_CONTROL_UNKNOWN;
  toggle_buttons_cb (NULL, &button_data);

  return TRUE;
}

static void
toggle_buttons_cb (NbtkButton *button, gpointer data)
{
  struct button_data         *button_data = data;
  MutterPlugin               *plugin = button_data->plugin;
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gint                        i;
  MnbkControl                 control = button_data->control;

  for (i = 0; i < G_N_ELEMENTS (priv->panel_buttons); i++)
    if (priv->panel_buttons[i] != (ClutterActor*)button)
      nbtk_button_set_checked (NBTK_BUTTON (priv->panel_buttons[i]), FALSE);

  if (control != MNBK_CONTROL_UNKNOWN)
    {
      gboolean active = nbtk_button_get_checked (button);

      /*
       * If we showing some UI element, we forcefully close any tray config
       * windows.
       */
      if (active)
        shell_tray_manager_close_all_config_windows (priv->tray_manager);

    }
}

static ClutterActor*
panel_append_toolbar_button (MutterPlugin  *plugin,
                             ClutterActor  *container,
                             const gchar   *name,
                             const gchar   *tooltip,
                             MnbkControl    control)
{
  static int n_buttons = 0;

  NbtkWidget *button;
  struct button_data *button_data = g_new (struct button_data, 1);

  button_data->control = control;
  button_data->plugin  = plugin;

  button = mnb_panel_button_new ();
  nbtk_button_set_toggle_mode (NBTK_BUTTON (button), TRUE);
  nbtk_widget_set_tooltip_text (button, tooltip);
  clutter_actor_set_name (CLUTTER_ACTOR (button), name);
  clutter_actor_set_size (CLUTTER_ACTOR (button), BUTTON_WIDTH, BUTTON_HEIGHT);
  clutter_actor_set_position (CLUTTER_ACTOR (button),
                              213 + (BUTTON_WIDTH * n_buttons)
                              + (BUTTON_SPACING * n_buttons),
                              PANEL_HEIGHT - BUTTON_HEIGHT);
  mnb_panel_button_set_reactive_area (MNB_PANEL_BUTTON (button),
                                      0,
                                      -(PANEL_HEIGHT - BUTTON_HEIGHT),
                                      BUTTON_WIDTH,
                                      PANEL_HEIGHT);

  clutter_container_add_actor (CLUTTER_CONTAINER (container),
                               CLUTTER_ACTOR (button));

  g_signal_connect_data (button, "clicked", G_CALLBACK (toggle_buttons_cb),
                         button_data, (GClosureNotify)g_free, 0);

  n_buttons++;
  return CLUTTER_ACTOR (button);
}

static gboolean
update_time_date (MoblinNetbookPluginPrivate *priv)
{
  time_t         t;
  struct tm     *tmp;
  char           time_str[64];

  t = time (NULL);
  tmp = localtime (&t);
  if (tmp)
    strftime (time_str, 64, "%l:%M %P", tmp);
  else
    snprintf (time_str, 64, "Time");
  nbtk_label_set_text (NBTK_LABEL (priv->panel_time), time_str);

  if (tmp)
    strftime (time_str, 64, "%B %e, %Y", tmp);
  else
    snprintf (time_str, 64, "Date");
  nbtk_label_set_text (NBTK_LABEL (priv->panel_date), time_str);

  return TRUE;
}

static gboolean
start_update_time_date (MoblinNetbookPluginPrivate *priv)
{
  update_time_date (priv);
  g_timeout_add_seconds (60, (GSourceFunc) update_time_date, priv);

  return FALSE;
}

static void
shell_tray_manager_icon_added (ShellTrayManager *mgr,
                               ClutterActor     *icon,
                               MutterPlugin     *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  const gchar                *name;
  const gchar                *tooltip = NULL;
  gint                        col = -1;
  gint                        screen_width, screen_height;
  gint                        x, y;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  name = clutter_actor_get_name (icon);

  if (!name || !*name)
    return;

  if (!strcmp (name, "tray-button-bluetooth"))
    {
      col = 0;
      tooltip = _("bluetooth");
    }
  else if (!strcmp (name, "tray-button-wifi"))
    {
      col = 3;
      tooltip = _("networks");
    }
  else if (!strcmp (name, "tray-button-sound"))
    {
      col = 2;
      tooltip = _("volume");
    }
  else if (!strcmp (name, "tray-button-battery"))
    {
      col = 1;
      tooltip = _("power & brightness");
    }
  else if (!strcmp (name, "tray-button-test"))
    {
      col = 4;
      tooltip = _("test");
    }

  if (col < 0)
    return;

  if (tooltip)
    nbtk_widget_set_tooltip_text (NBTK_WIDGET (icon), tooltip);

  y = PANEL_HEIGHT - TRAY_BUTTON_HEIGHT;
  x = screen_width - (col + 1) * (TRAY_BUTTON_WIDTH + TRAY_PADDING);

  clutter_actor_set_position (icon, x, y);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->panel), icon);
}

static void
shell_tray_manager_icon_removed (ShellTrayManager *mgr,
                                 ClutterActor     *icon,
                                 MutterPlugin     *plugin)
{
  clutter_actor_destroy (icon);
}

static gboolean
panel_enter_event_cb (ClutterActor *panel, ClutterEvent *event, gpointer data)
{
  MutterPlugin               *plugin = MUTTER_PLUGIN (data);
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (event->any.type != CLUTTER_ENTER)
    return FALSE;

  priv->panel_wait_for_pointer = FALSE;

  return FALSE;
}

enum
{
  PANEL_PAGE_M_ZONE             = 0,
  PANEL_PAGE_STATUS             = 1,
  PANEL_PAGE_SPACES             = 2,
  PANEL_PAGE_INTERNET           = 3,
  PANEL_PAGE_MEDIA              = 4,
  PANEL_PAGE_APPS               = 5,
  PANEL_PAGE_PEOPLE             = 6,
  PANEL_PAGE_PASTERBOARD        = 7
};

static inline void
make_toolbar_button (MutterPlugin *plugin,
                     ClutterActor *panel,
                     const gchar  *icon_name,
                     const gchar  *name,
                     MnbkControl   control,
                     gint          index_)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  ClutterActor *button;

  button = panel_append_toolbar_button (plugin, panel,
                                        icon_name, name,
                                        control);
  priv->panel_buttons[index_] = button;
}

static void
_mzone_activated_cb (PengeGridView *view, gpointer userdata)
{
  MoblinNetbookPlugin *plugin = MOBLIN_NETBOOK_PLUGIN (userdata);
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  clutter_actor_hide (priv->mzone_grid);
  hide_panel ((MutterPlugin *)plugin);
}

#ifdef USE_AHOGHILL
/* FIXME: Focus should really be done by the panel */
static void
_media_drop_down_hidden (MnbDropDown      *drop_down,
                         AhoghillGridView *view)
{
  ahoghill_grid_view_clear (view);
  ahoghill_grid_view_unfocus (view);
}

static void
_media_drop_down_shown (MnbDropDown      *drop_down,
                        AhoghillGridView *view)
{
  ahoghill_grid_view_focus (view);
}

#endif

#ifdef WITH_NETPANEL
static void
_netgrid_launch_cb (MoblinNetbookNetpanel *netpanel,
                    const gchar           *url,
                    MutterPlugin          *plugin)
{
  gchar *exec, *esc_url;
  gint workspace;

  /* FIXME: Should not be hard-coded? */
  esc_url = g_strescape (url, NULL);
  exec = g_strdup_printf ("%s \"%s\"", "moblin-web-browser", esc_url);

  workspace =
    meta_screen_get_active_workspace_index (mutter_plugin_get_screen (plugin));
  moblin_netbook_launch_application (plugin, exec, TRUE, workspace);

  g_free (exec);
  g_free (esc_url);

  hide_panel (plugin);
}
#endif

ClutterActor *
make_panel (MutterPlugin *plugin, gint width)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  ClutterActor               *panel;
  ClutterActor               *shadow;
  ClutterActor               *background, *bg_texture;
  ClutterColor                clr = {0x0, 0x0, 0x0, 0xce};
  ClutterActor               *overlay;
  ClutterActor               *mzone_grid_view;
#ifdef USE_AHOGHILL
  ClutterActor               *ahoghill_grid_view;
#endif
#ifdef WITH_NETPANEL
  ClutterActor               *net_grid_view;
#endif
  GError                     *err = NULL;
  gint                        screen_width, screen_height;
  time_t         t;
  struct tm     *tmp;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  overlay = mutter_plugin_get_overlay_group (plugin);

  panel = clutter_group_new ();
  clutter_actor_set_reactive (panel, TRUE);

  g_signal_connect (panel, "captured-event",
                    G_CALLBACK (panel_enter_event_cb), plugin);

  /* FIME -- size and color */
  bg_texture = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                              "/theme/panel/panel-shadow.png",
                                              &err);
  if (err)
    {
      g_warning ("%s", err->message);
      g_clear_error (&err);
    }
  else
    {
      shadow = nbtk_texture_frame_new (CLUTTER_TEXTURE (bg_texture),
                                       0,   /* top */
                                       200, /* right */
                                       0,   /* bottom */
                                       200  /* left */);
      clutter_actor_set_size (shadow, width, 101);
      clutter_container_add_actor (CLUTTER_CONTAINER (panel), shadow);
      priv->panel_shadow = shadow;
    }

  bg_texture = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                            "/theme/panel/panel-background.png",
                                              &err);
  if (err)
    {
      g_warning ("%s", err->message);
      g_clear_error (&err);
    }
  else
    {
      background = nbtk_texture_frame_new (CLUTTER_TEXTURE (bg_texture),
                                           0,   /* top */
                                           200, /* right */
                                           0,   /* bottom */
                                           200  /* left */);
      clutter_actor_set_size (background, width - 8, PANEL_HEIGHT);
      clutter_actor_set_x (background, 4);
      clutter_container_add_actor (CLUTTER_CONTAINER (panel), background);
    }

  make_toolbar_button (plugin, panel,
                       "m-space-button",
                       _("m_zone"),
                       MNBK_CONTROL_MZONE,
                       PANEL_PAGE_M_ZONE);
  nbtk_button_set_checked (NBTK_BUTTON (priv->panel_buttons[PANEL_PAGE_M_ZONE]),
                          TRUE);

  make_toolbar_button (plugin, panel,
                       "status-button",
                       _("status"),
                       MNBK_CONTROL_STATUS,
                       PANEL_PAGE_STATUS);

  make_toolbar_button (plugin, panel,
                       "people-button",
                       _("people"),
                       MNBK_CONTROL_PEOPLE,
                       PANEL_PAGE_PEOPLE);

  make_toolbar_button (plugin, panel,
                       "internet-button",
                       _("internet"),
                       MNBK_CONTROL_INTERNET,
                       PANEL_PAGE_INTERNET);

  make_toolbar_button (plugin, panel,
                       "media-button",
                       _("media"),
                       MNBK_CONTROL_MEDIA,
                       PANEL_PAGE_MEDIA);

  make_toolbar_button (plugin, panel,
                       "pasteboard-button",
                       _("pasteboard"),
                       MNBK_CONTROL_PASTEBOARD,
                       PANEL_PAGE_PASTERBOARD);

  make_toolbar_button (plugin, panel,
                       "apps-button",
                       _("applications"),
                       MNBK_CONTROL_APPLICATIONS,
                       PANEL_PAGE_APPS);

  make_toolbar_button (plugin, panel,
                       "spaces-button",
                       _("zones"),
                       MNBK_CONTROL_SPACES,
                       PANEL_PAGE_SPACES);

  priv->panel_time = nbtk_label_new ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->panel_time), "time-label");
  priv->panel_date = nbtk_label_new ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->panel_date), "date-label");
  update_time_date (priv);

  clutter_container_add_actor (CLUTTER_CONTAINER (panel),
                               CLUTTER_ACTOR (priv->panel_time));
  clutter_actor_set_position (CLUTTER_ACTOR (priv->panel_time),
       (192 / 2) - clutter_actor_get_width (CLUTTER_ACTOR (priv->panel_time)) /
                              2, 8);


  clutter_container_add_actor (CLUTTER_CONTAINER (panel),
                               CLUTTER_ACTOR (priv->panel_date));
  clutter_actor_set_position (CLUTTER_ACTOR (priv->panel_date),
       (192 / 2) - clutter_actor_get_width (CLUTTER_ACTOR (priv->panel_date)) /
                              2, 40);

  /* update the clock at :00 seconds */
  t = time (NULL);
  tmp = localtime (&t);
  g_timeout_add_seconds (60 - tmp->tm_sec,
                         (GSourceFunc) start_update_time_date,
                         priv);

  /* status drop down */
  priv->status = make_status (plugin, width - PANEL_X_PADDING * 2);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), priv->status);
  mnb_drop_down_set_button (MNB_DROP_DOWN (priv->status),
                            NBTK_BUTTON (priv->panel_buttons[PANEL_PAGE_STATUS]));
  clutter_actor_set_width (priv->status, screen_width);
  clutter_actor_set_position (priv->status, 0, PANEL_HEIGHT);
  clutter_actor_lower_bottom (priv->status);

  /* switcher drop down */
  priv->switcher = (ClutterActor *) mnb_switcher_new (plugin);
  clutter_container_add (CLUTTER_CONTAINER (panel),
                         CLUTTER_ACTOR (priv->switcher), NULL);
  mnb_drop_down_set_button (MNB_DROP_DOWN (priv->switcher),
                            NBTK_BUTTON (priv->panel_buttons[PANEL_PAGE_SPACES]));
  clutter_actor_set_width (priv->switcher, screen_width);
  clutter_actor_set_position (priv->switcher, 0, PANEL_HEIGHT);
  clutter_actor_lower_bottom (priv->switcher);
  clutter_actor_hide (priv->switcher);

  /* launcher drop down */
  priv->launcher = make_launcher (plugin,
                                  width - PANEL_X_PADDING * 2,
                                  screen_height - 2 * PANEL_HEIGHT);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), priv->launcher);
  mnb_drop_down_set_button (MNB_DROP_DOWN (priv->launcher),
                            NBTK_BUTTON (priv->panel_buttons[PANEL_PAGE_APPS]));
  clutter_actor_set_position (priv->launcher, 0, PANEL_HEIGHT);
  clutter_actor_set_width (priv->launcher, screen_width);
  clutter_actor_lower_bottom (priv->launcher);
  clutter_actor_hide (priv->launcher);

  /* pasteboard drop down */
  priv->pasteboard = make_pasteboard (plugin,
                                      width - PANEL_X_PADDING * 2);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel),
                               priv->pasteboard);
  mnb_drop_down_set_button (MNB_DROP_DOWN (priv->pasteboard),
                            NBTK_BUTTON (priv->panel_buttons[PANEL_PAGE_PASTERBOARD]));
  clutter_actor_set_position (priv->pasteboard, 0, PANEL_HEIGHT);
  clutter_actor_set_width (priv->pasteboard, screen_width);
  clutter_actor_lower_bottom (priv->pasteboard);
  clutter_actor_hide (priv->pasteboard);

  priv->tray_manager = g_object_new (SHELL_TYPE_TRAY_MANAGER,
                                     "bg-color", &clr,
                                     "mutter-plugin", plugin,
                                     NULL);

  g_signal_connect (priv->tray_manager, "tray-icon-added",
                    G_CALLBACK (shell_tray_manager_icon_added), plugin);

  g_signal_connect (priv->tray_manager, "tray-icon-removed",
                    G_CALLBACK (shell_tray_manager_icon_removed), plugin);

  shell_tray_manager_manage_stage (priv->tray_manager,
                                   CLUTTER_STAGE (
                                           mutter_plugin_get_stage (plugin)));

  /* m-zone drop down */
  priv->mzone_grid = CLUTTER_ACTOR (mnb_drop_down_new ());
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), priv->mzone_grid);
  clutter_actor_set_width (priv->mzone_grid, screen_width);
  mzone_grid_view = g_object_new (PENGE_TYPE_GRID_VIEW, NULL);
  g_signal_connect (mzone_grid_view, "activated",
                    G_CALLBACK (_mzone_activated_cb), plugin);
  clutter_actor_set_height (mzone_grid_view, screen_height - PANEL_HEIGHT * 1.5);
  mnb_drop_down_set_child (MNB_DROP_DOWN (priv->mzone_grid),
                           CLUTTER_ACTOR (mzone_grid_view));
  mnb_drop_down_set_button (MNB_DROP_DOWN (priv->mzone_grid),
                            NBTK_BUTTON (priv->panel_buttons[PANEL_PAGE_M_ZONE]));
  clutter_actor_set_position (priv->mzone_grid, 0, PANEL_HEIGHT);
  clutter_actor_lower_bottom (priv->mzone_grid);

#ifdef USE_AHOGHILL
  /* Ahoghill media drop down */
  priv->media_drop_down = CLUTTER_ACTOR (mnb_drop_down_new ());
  clutter_container_add_actor (CLUTTER_CONTAINER (panel),
                               priv->media_drop_down);
  clutter_actor_set_width (priv->media_drop_down,
                           screen_width - 2 * PANEL_X_PADDING);

  ahoghill_grid_view = g_object_new (AHOGHILL_TYPE_GRID_VIEW, NULL);
  clutter_actor_set_height (mzone_grid_view,
                            screen_height - PANEL_HEIGHT * 1.5);
  mnb_drop_down_set_child (MNB_DROP_DOWN (priv->media_drop_down),
                           CLUTTER_ACTOR (ahoghill_grid_view));
  mnb_drop_down_set_button (MNB_DROP_DOWN (priv->media_drop_down),
                            NBTK_BUTTON (priv->panel_buttons[PANEL_PAGE_MEDIA]));
  clutter_actor_set_position (priv->media_drop_down, 0, PANEL_HEIGHT);
  clutter_actor_lower_bottom (priv->media_drop_down);

  g_signal_connect (priv->media_drop_down, "hide-completed",
                    G_CALLBACK (_media_drop_down_hidden),
                    ahoghill_grid_view);
  g_signal_connect (priv->media_drop_down, "show-completed",
                    G_CALLBACK (_media_drop_down_shown),
                    ahoghill_grid_view);
  g_signal_connect_swapped (ahoghill_grid_view, "dismiss",
                            G_CALLBACK (hide_panel), plugin);

#endif

#ifdef WITH_NETPANEL
  /* Internet panel drop-down */
  priv->net_grid = CLUTTER_ACTOR (mnb_drop_down_new ());
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), priv->net_grid);
  clutter_actor_set_width (priv->net_grid, screen_width);
  net_grid_view = CLUTTER_ACTOR (moblin_netbook_netpanel_new ());
  mnb_drop_down_set_child (MNB_DROP_DOWN (priv->net_grid),
                           CLUTTER_ACTOR (net_grid_view));
  mnb_drop_down_set_button (MNB_DROP_DOWN (priv->net_grid),
                            NBTK_BUTTON (priv->panel_buttons[PANEL_PAGE_INTERNET]));
  clutter_actor_set_position (priv->net_grid, 0, PANEL_HEIGHT);
  clutter_actor_lower_bottom (priv->net_grid);

  g_signal_connect_swapped (panel, "hide",
                            G_CALLBACK (clutter_actor_hide), net_grid_view);
  g_signal_connect_swapped (panel, "show",
                            G_CALLBACK (clutter_actor_show), net_grid_view);
  g_signal_connect_swapped (priv->net_grid, "hide-completed",
                            G_CALLBACK (moblin_netbook_netpanel_clear),
                            net_grid_view);
  g_signal_connect_swapped (priv->net_grid, "show-completed",
                            G_CALLBACK (moblin_netbook_netpanel_focus),
                            net_grid_view);
  g_signal_connect (net_grid_view, "launch",
                    G_CALLBACK (_netgrid_launch_cb), plugin);
  g_signal_connect_swapped (net_grid_view, "launched",
                            G_CALLBACK (hide_panel), plugin);
#endif

#ifdef WITH_PEOPLE
  /* people drop down */
  priv->people = make_people_panel (plugin,
                                    width - PANEL_X_PADDING * 2);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel),
                               priv->people);
  mnb_drop_down_set_button (MNB_DROP_DOWN (priv->people),
                            NBTK_BUTTON (priv->panel_buttons[PANEL_PAGE_PEOPLE]));
  clutter_actor_set_position (priv->people, 0, PANEL_HEIGHT);
  clutter_actor_set_width (priv->people, screen_width);
  clutter_actor_lower_bottom (priv->people);
  clutter_actor_hide (priv->people);
#endif

  if (shadow)
    clutter_actor_lower_bottom (shadow);

  return panel;
}

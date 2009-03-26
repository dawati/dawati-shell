/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych    <tf@linux.intel.com>
 *         Chris Lord        <christopher.lord@intel.com>
 *         Robert Staudinger <robertx.staudinger@intel.com>
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

#include <gtk/gtk.h>
#include <nbtk/nbtk.h>

#include <penge/penge-app-bookmark-manager.h>

#include "moblin-netbook.h"
#include "moblin-netbook-chooser.h"
#include "moblin-netbook-launcher.h"
#include "moblin-netbook-panel.h"
#include "mnb-drop-down.h"
#include "mnb-entry.h"
#include "mnb-expander.h"
#include "mnb-launcher-button.h"
#include "mnb-launcher-tree.h"

#define WIDGET_SPACING 5
#define ICON_SIZE 48
#define PADDING 8
#define N_COLS 4
#define LAUNCHER_WIDTH  235
#define LAUNCHER_HEIGHT 64

/*
 * Helper struct that contains all the info needed to switch between
 * browser- and filter-mode.
 */
typedef struct
{
  MutterPlugin            *self;
  PengeAppBookmarkManager *manager;
  MnbLauncherMonitor      *monitor;
  ClutterActor            *scrolled_vbox;
  ClutterActor            *fav_grid;
  ClutterActor            *apps_grid;
  GHashTable              *expanders;
  GSList                  *launchers;
  gboolean                 is_filtering;
  GSList                  *launchers_iter;
  char                    *lcase_needle;
} launcher_data_t;

static void launcher_data_monitor_cb (MnbLauncherMonitor  *monitor,
                                      launcher_data_t     *launcher_data);

static void
launcher_activated_cb (MnbLauncherButton  *launcher,
                       ClutterButtonEvent *event,
                       MutterPlugin       *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  const gchar                *exec = mnb_launcher_button_get_executable (launcher);
  gchar                      *last_used;
  gboolean                    without_chooser = FALSE;
  gint                        workspace       = -2;

  MetaScreen *screen = mutter_plugin_get_screen (plugin);
  gint        n_ws   = meta_screen_get_n_workspaces (screen);
  gboolean    empty  = FALSE;

  if (n_ws == 1)
    {
      GList * l;

      empty = TRUE;

      l = mutter_get_windows (screen);
      while (l)
        {
          MutterWindow *m    = l->data;
          MetaWindow   *w = mutter_window_get_meta_window (m);

          if (w)
            {
              MetaCompWindowType type = mutter_window_get_window_type (m);

              if (type == META_COMP_WINDOW_NORMAL)
                {
                  empty = FALSE;
                  break;
                }
            }

          l = l->next;
        }
    }

  if (empty)
    {
      without_chooser = TRUE;
      workspace = 0;
    }

  moblin_netbook_spawn (plugin, exec, event->time, without_chooser, workspace);

  last_used = mnb_launcher_utils_get_last_used (exec);
  mnb_launcher_button_set_comment (launcher, last_used);
  g_free (last_used);

  clutter_actor_hide (priv->launcher);
  nbtk_button_set_checked (NBTK_BUTTON (priv->panel_buttons[5]), FALSE);
  hide_panel (plugin);
}

static void
launcher_fav_toggled_cb (MnbLauncherButton  *launcher,
                         launcher_data_t    *launcher_data)
{
  printf ("%s\n", mnb_launcher_button_get_desktop_file_path (launcher));
/* TODO */
  if (mnb_launcher_button_get_favorite (launcher))
    {
    
    }
  else
    {
    
    }
}

static NbtkWidget *
create_launcher_button_from_entry (MnbLauncherEntry *entry,
                                   const gchar      *category,
                                   GtkIconTheme     *theme)
{
  const gchar *generic_name, *icon_file;
  gchar       *description, *exec, *icon_name;
  GtkIconInfo *info;
  NbtkWidget  *button;

  description = NULL;
  exec = NULL;
  icon_name = NULL;
  info = NULL;
  button = NULL;

  generic_name = mnb_launcher_entry_get_name (entry);
  exec = mnb_launcher_entry_get_exec (entry);
  description = mnb_launcher_entry_get_comment (entry);
  icon_name = mnb_launcher_entry_get_icon (entry);
  if (icon_name)
    {
      info = gtk_icon_theme_lookup_icon (theme,
                                          icon_name,
                                          ICON_SIZE,
                                          GTK_ICON_LOOKUP_GENERIC_FALLBACK);
    }
  if (!info)
    {
      info = gtk_icon_theme_lookup_icon (theme,
                                          "applications-other",
                                          ICON_SIZE,
                                          GTK_ICON_LOOKUP_GENERIC_FALLBACK);
    }
  if (info)
    {
      icon_file = gtk_icon_info_get_filename (info);
    }

  if (generic_name && exec && icon_file)
    {
      gchar *last_used;

      /* Launcher button */
      last_used = mnb_launcher_utils_get_last_used (exec);
      button = mnb_launcher_button_new (icon_file, ICON_SIZE,
                                        generic_name, category,
                                        description, last_used, exec,
                                        mnb_launcher_entry_get_desktop_file_path (entry));
      g_free (last_used);
      clutter_actor_set_size (CLUTTER_ACTOR (button),
                              LAUNCHER_WIDTH,
                              LAUNCHER_HEIGHT);
    }

  g_free (description);
  g_free (icon_name);
  g_free (exec);
  if (info)
    gtk_icon_info_free (info);

  return button;
}

static void
expander_notify_cb (MnbExpander    *expander,
                    GParamSpec      *pspec,
                    launcher_data_t *launcher_data)
{
  MnbExpander    *e;
  const gchar     *category;
  GHashTableIter   iter;

  /* Close other open expander, so that just the newly opended one is expanded. */
  if (mnb_expander_get_expanded (expander))
    {
      g_hash_table_iter_init (&iter, launcher_data->expanders);
      while (g_hash_table_iter_next (&iter,
                                     (gpointer *) &category,
                                     (gpointer *) &e))
        {
          if (e != expander)
            mnb_expander_set_expanded (e, FALSE);
        }
    }
}

static void
launcher_data_cancel_search (launcher_data_t *launcher_data)
{
  /* Abort current search if any. */
  if (launcher_data->lcase_needle)
    {
      g_idle_remove_by_data (launcher_data);
      launcher_data->launchers_iter = NULL;
      g_free (launcher_data->lcase_needle);
      launcher_data->lcase_needle = NULL;
    }
}

static void
launcher_data_reset (launcher_data_t *launcher_data)
{
  launcher_data_cancel_search (launcher_data);

  clutter_actor_destroy (launcher_data->scrolled_vbox);
  launcher_data->scrolled_vbox = NULL;

  g_hash_table_destroy (launcher_data->expanders);
  launcher_data->expanders = NULL;

  g_slist_free (launcher_data->launchers);
  launcher_data->launchers = NULL;
}

static void
launcher_data_fill (launcher_data_t *launcher_data)
{
  GList           *fav_apps;
  ClutterActor    *label;
  MnbLauncherTree *tree;
  GSList          *directories;
  GSList const    *directory_iter;
  GtkIconTheme    *theme;
  gint             row = 0;

  theme = gtk_icon_theme_get_default ();

  launcher_data->scrolled_vbox = CLUTTER_ACTOR (nbtk_table_new ());

  /*
   * Fav apps.
   */
  fav_apps = penge_app_bookmark_manager_get_bookmarks (launcher_data->manager);
  if (fav_apps)
    {
      GList *fav_apps_iter;

      /* Label */
      label = CLUTTER_ACTOR (nbtk_label_new (_("Favourite Applications")));
      clutter_actor_set_name (label, "app-launcher-apps-label");
      nbtk_table_add_widget_full (NBTK_TABLE (launcher_data->scrolled_vbox),
                                  NBTK_WIDGET (label),
                                  row++, 0, 1, 1,
                                  NBTK_X_EXPAND | NBTK_Y_EXPAND | NBTK_X_FILL | NBTK_Y_FILL,
                                  0., 0.);

      /* Grid */
      launcher_data->fav_grid = CLUTTER_ACTOR (nbtk_grid_new ());
      nbtk_table_add_widget_full (NBTK_TABLE (launcher_data->scrolled_vbox),
                                  NBTK_WIDGET (launcher_data->fav_grid),
                                  row++, 0, 1, 1,
                                  NBTK_X_EXPAND | NBTK_Y_EXPAND | NBTK_X_FILL | NBTK_Y_FILL,
                                  0., 0.);

      for (fav_apps_iter = fav_apps;
           fav_apps_iter;
           fav_apps_iter = fav_apps_iter->next)
        {
          PengeAppBookmark  *bookmark;
          gchar             *desktop_file_path;
          MnbLauncherEntry  *entry;
          NbtkWidget        *button = NULL;
          GError            *error = NULL;

          bookmark = (PengeAppBookmark *) fav_apps_iter->data;
          desktop_file_path = g_filename_from_uri (bookmark->uri, NULL, &error);
          if (error)
            {
              g_warning ("%s", error->message);
              g_clear_error (&error);
              continue;
            }
          
          entry = mnb_launcher_entry_create (desktop_file_path);
          g_free (desktop_file_path);
          if (entry)
            {
              button = create_launcher_button_from_entry (entry, NULL, theme);
              mnb_launcher_entry_free (entry);            
            }

          if (button)
            {
              mnb_launcher_button_set_favorite (MNB_LAUNCHER_BUTTON (button),
                                                TRUE);
              clutter_container_add (CLUTTER_CONTAINER (launcher_data->fav_grid),
                                     CLUTTER_ACTOR (button), NULL);
              g_signal_connect (button, "activated",
                                G_CALLBACK (launcher_activated_cb),
                                launcher_data->self);
              g_signal_connect (button, "fav-toggled",
                                G_CALLBACK (launcher_fav_toggled_cb),
                                launcher_data);
            }
        }
      g_list_free (fav_apps);
    }

  /*
   * Apps browser.
   */

  /* Label */
  label = CLUTTER_ACTOR (nbtk_label_new (_("Applications")));
  clutter_actor_set_name (label, "app-launcher-apps-label");
  nbtk_table_add_widget_full (NBTK_TABLE (launcher_data->scrolled_vbox),
                              NBTK_WIDGET (label),
                              row++, 0, 1, 1,
                              NBTK_X_EXPAND | NBTK_Y_EXPAND | NBTK_X_FILL | NBTK_Y_FILL,
                              0., 0.);

  /* Grid */
  launcher_data->apps_grid = CLUTTER_ACTOR (nbtk_grid_new ());
  clutter_actor_set_width (launcher_data->apps_grid,
                           4 * LAUNCHER_WIDTH + 5 * PADDING);
  clutter_actor_set_name (launcher_data->apps_grid, "app-launcher-table");
  nbtk_grid_set_row_gap (NBTK_GRID (launcher_data->apps_grid),
                         CLUTTER_UNITS_FROM_INT (PADDING));
  nbtk_grid_set_column_gap (NBTK_GRID (launcher_data->apps_grid),
                            CLUTTER_UNITS_FROM_INT (PADDING));
  nbtk_table_add_widget_full (NBTK_TABLE (launcher_data->scrolled_vbox),
                              NBTK_WIDGET (launcher_data->apps_grid),
                              row++, 0, 1, 1,
                              NBTK_X_EXPAND | NBTK_Y_EXPAND | NBTK_X_FILL | NBTK_Y_FILL,
                              0., 0.);

  launcher_data->expanders = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                    g_free, NULL);

  tree = mnb_launcher_tree_create ();
  directories = mnb_launcher_tree_list_entries (tree);

  for (directory_iter = directories;
       directory_iter;
       directory_iter = directory_iter->next)
    {
      MnbLauncherDirectory  *directory;
      GSList                *entry_iter;
      ClutterActor          *expander, *inner_grid;

      directory = (MnbLauncherDirectory *) directory_iter->data;

      /* Expander. */
      expander = CLUTTER_ACTOR (mnb_expander_new (directory->name));
      clutter_actor_set_width (expander, 4 * LAUNCHER_WIDTH + 5 * PADDING);
      clutter_container_add (CLUTTER_CONTAINER (launcher_data->apps_grid),
                             expander, NULL);
      g_hash_table_insert (launcher_data->expanders,
                           g_strdup (directory->name), expander);
      g_signal_connect (expander, "notify::expanded",
                        G_CALLBACK (expander_notify_cb), launcher_data);

      /* Open first expander by default. */
      if (directory_iter == directories)
        {
          mnb_expander_set_expanded (MNB_EXPANDER (expander), TRUE);
        }

      inner_grid = CLUTTER_ACTOR (nbtk_grid_new ());
      clutter_container_add (CLUTTER_CONTAINER (expander), inner_grid, NULL);

      for (entry_iter = directory->entries; entry_iter; entry_iter = entry_iter->next)
        {
          NbtkWidget *button;

          button = create_launcher_button_from_entry ((MnbLauncherEntry *) entry_iter->data,
                                                      directory->name,
                                                      theme);
          if (button)
            {
              clutter_container_add (CLUTTER_CONTAINER (inner_grid),
                                      CLUTTER_ACTOR (button), NULL);
              g_signal_connect (button, "activated",
                                G_CALLBACK (launcher_activated_cb),
                                launcher_data->self);
              g_signal_connect (button, "fav-toggled",
                                G_CALLBACK (launcher_fav_toggled_cb),
                                launcher_data);
              launcher_data->launchers = g_slist_prepend (launcher_data->launchers,
                                                          button);
            }
        }
    }

  /* Alphabetically sort buttons, so they are in order while filtering. */
  launcher_data->launchers = g_slist_sort (launcher_data->launchers,
                                           (GCompareFunc) mnb_launcher_button_compare);

  /* Create monitor only once. */
  if (!launcher_data->monitor)
    {
      launcher_data->monitor =
        mnb_launcher_tree_create_monitor (
          tree,
          (MnbLauncherMonitorFunction) launcher_data_monitor_cb,
           launcher_data);
    }

  mnb_launcher_tree_free_entries (directories);
  mnb_launcher_tree_free (tree);
}

/*
 * Ctor.
 */
static launcher_data_t *
launcher_data_new (MutterPlugin *self)
{
  launcher_data_t *launcher_data;

  /* Launcher data instance. */
  launcher_data = g_new0 (launcher_data_t, 1);
  launcher_data->self = self;
  launcher_data->manager = penge_app_bookmark_manager_get_default ();
  penge_app_bookmark_manager_load (launcher_data->manager);

  launcher_data_fill (launcher_data);

  return launcher_data;
}

/*
 * Dtor.
 */
static void
launcher_data_free_cb (launcher_data_t *launcher_data)
{
  g_object_unref (launcher_data->manager);
  mnb_launcher_monitor_free (launcher_data->monitor);

  launcher_data_reset (launcher_data);
  g_free (launcher_data);
}

static void
launcher_data_monitor_cb (MnbLauncherMonitor  *monitor,
                          launcher_data_t     *launcher_data)
{
  ClutterActor *viewport;

  viewport = clutter_actor_get_parent (launcher_data->scrolled_vbox);

  launcher_data_reset (launcher_data);
  launcher_data_fill (launcher_data);

  clutter_container_add (CLUTTER_CONTAINER (viewport),
                         CLUTTER_ACTOR (launcher_data->scrolled_vbox), NULL);
}

static gboolean
filter_cb (launcher_data_t *launcher_data)
{
  MnbLauncherButton *button;

  /* Start search? */
  if (launcher_data->launchers_iter == NULL)
    {
      g_return_val_if_fail (launcher_data->launchers, FALSE);
      launcher_data->launchers_iter = launcher_data->launchers;
    }

  button = MNB_LAUNCHER_BUTTON (launcher_data->launchers_iter->data);

  /* Do search. */
  mnb_launcher_button_match (button, launcher_data->lcase_needle) ?
    clutter_actor_show (CLUTTER_ACTOR (button)) :
    clutter_actor_hide (CLUTTER_ACTOR (button));

  launcher_data->launchers_iter = launcher_data->launchers_iter->next;

  /* Done searching? */
  if (launcher_data->launchers_iter == NULL)
    {
      g_free (launcher_data->lcase_needle);
      launcher_data->lcase_needle = NULL;
      return FALSE;
    }

  return TRUE;
}

static void
search_activated_cb (MnbEntry         *entry,
                     launcher_data_t  *launcher_data)
{
  const gchar *needle;

  launcher_data_cancel_search (launcher_data);

  needle = mnb_entry_get_text (entry);

  if (needle && strlen (needle) > 0)
    {
      /* Do filter */

      gchar *lcase_needle = g_utf8_strdown (needle, -1);

      /* Need to switch to filter mode? */
      if (!launcher_data->is_filtering)
        {
          GSList          *iter;
          GHashTableIter   expander_iter;
          ClutterActor    *expander;

          launcher_data->is_filtering = TRUE;

          /* Hide expanders. */
          g_hash_table_iter_init (&expander_iter, launcher_data->expanders);
          while (g_hash_table_iter_next (&expander_iter,
                                         NULL,
                                         (gpointer *) &expander))
            {
              clutter_actor_hide (expander);
            }

          /* Reparent launchers onto grid.
           * Launchers are initially invisible to avoid bogus matches. */
          for (iter = launcher_data->launchers; iter; iter = iter->next)
            {
              MnbLauncherButton *launcher = MNB_LAUNCHER_BUTTON (iter->data);
              clutter_actor_hide (CLUTTER_ACTOR (launcher));
              clutter_actor_reparent (CLUTTER_ACTOR (launcher),
                                      launcher_data->apps_grid);
            }
        }

      /* Update search result. */
      launcher_data->lcase_needle = g_strdup (lcase_needle);
      g_idle_add ((GSourceFunc) filter_cb, launcher_data);

      g_free (lcase_needle);
    }
  else if (launcher_data->is_filtering &&
           (!needle || strlen (needle) == 0))
    {
      /* Did filter, now switch back to normal mode */
      GSList          *iter;
      GHashTableIter   expander_iter;
      ClutterActor    *expander;

      launcher_data->is_filtering = FALSE;

      /* Reparent launchers into expanders. */
      for (iter = launcher_data->launchers; iter; iter = iter->next)
        {
          MnbLauncherButton *launcher   = MNB_LAUNCHER_BUTTON (iter->data);
          const gchar       *category   = mnb_launcher_button_get_category (launcher);
          ClutterActor      *e          = g_hash_table_lookup (launcher_data->expanders, category);
          ClutterActor      *inner_grid = mnb_expander_get_child (MNB_EXPANDER (e));

          clutter_actor_reparent (CLUTTER_ACTOR (launcher), inner_grid);
        }

      /* Show expanders. */
      g_hash_table_iter_init (&expander_iter, launcher_data->expanders);
      while (g_hash_table_iter_next (&expander_iter, NULL, (gpointer *) &expander))
        {
          clutter_actor_show (expander);
        }
    }
}

static void
dropdown_show_cb (MnbDropDown   *dropdown,
                  ClutterActor  *filter_entry)
{
  clutter_actor_grab_key_focus (filter_entry);
}

static void
dropdown_hide_cb (MnbDropDown   *dropdown,
                  ClutterActor  *filter_entry)
{
  /* Reset search. */
  mnb_entry_set_text (MNB_ENTRY (filter_entry), "");
}

ClutterActor *
make_launcher (MutterPlugin *plugin,
               gint          width,
               gint          height)
{
  ClutterActor    *viewport, *scroll;
  NbtkWidget      *vbox, *hbox, *label, *entry, *drop_down;
  launcher_data_t *launcher_data;

  drop_down = mnb_drop_down_new ();

  vbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "app-launcher-vbox");
  nbtk_table_set_row_spacing (NBTK_TABLE (vbox), WIDGET_SPACING);
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), CLUTTER_ACTOR (vbox));

  /* Filter row. */
  hbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (hbox), "app-launcher-search");
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 20);
  nbtk_table_add_widget (NBTK_TABLE (vbox), hbox, 0, 0);

  label = nbtk_label_new (_("Applications"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "app-launcher-search-label");
  nbtk_table_add_widget_full (NBTK_TABLE (hbox), label,
                              0, 0, 1, 1,
                              0,
                              0., 0.5);

  entry = mnb_entry_new (_("Search"));
  clutter_actor_set_name (CLUTTER_ACTOR (entry), "app-launcher-search-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (entry),
                           CLUTTER_UNITS_FROM_DEVICE (600));
  nbtk_table_add_widget_full (NBTK_TABLE (hbox), entry,
                              0, 1, 1, 1,
                              0,
                              0., 0.5);
  g_signal_connect (drop_down, "show-completed",
                    G_CALLBACK (dropdown_show_cb), entry);
  g_signal_connect (drop_down, "hide-completed",
                    G_CALLBACK (dropdown_hide_cb), entry);

  /*
   * Applications
   */
  viewport = CLUTTER_ACTOR (nbtk_viewport_new ());
  /* Add launcher table. */
  launcher_data = launcher_data_new (plugin);
  clutter_container_add (CLUTTER_CONTAINER (viewport),
                         CLUTTER_ACTOR (launcher_data->scrolled_vbox), NULL);

  scroll = CLUTTER_ACTOR (nbtk_scroll_view_new ());
  clutter_container_add (CLUTTER_CONTAINER (scroll),
                         CLUTTER_ACTOR (viewport), NULL);
  clutter_actor_set_size (scroll,
                          width,
                          height - clutter_actor_get_height (CLUTTER_ACTOR (entry)) - WIDGET_SPACING);
  nbtk_table_add_widget_full (NBTK_TABLE (vbox), NBTK_WIDGET (scroll),
                              1, 0, 1, 1,
                              NBTK_X_EXPAND | NBTK_Y_EXPAND | NBTK_X_FILL | NBTK_Y_FILL,
                              0., 0.);

  /* Hook up search. */
  g_signal_connect_data (entry, "button-clicked",
                         G_CALLBACK (search_activated_cb), launcher_data,
                         (GClosureNotify) launcher_data_free_cb, 0);
  /* `launcher_data' lifecycle is managed above. */
  g_signal_connect (entry, "text-changed",
                    G_CALLBACK (search_activated_cb), launcher_data);

  return CLUTTER_ACTOR (drop_down);
}

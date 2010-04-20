/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych    <tf@linux.intel.com>
 *         Chris Lord        <christopher.lord@intel.com>
 *         Robert Staudinger <robertx.staudinger@intel.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>

#include <gtk/gtk.h>
#include <mx/mx.h>
#include <moblin-panel/mpl-icon-theme.h>

#include <moblin-panel/mpl-app-bookmark-manager.h>
#include <moblin-panel/mpl-content-pane.h>

#include "moblin-netbook-launcher.h"
#include "mnb-filter.h"
#include "mnb-expander.h"
#include "mnb-launcher-button.h"
#include "mnb-launcher-grid.h"
#include "mnb-launcher-tree.h"

#define SEARCH_APPLY_TIMEOUT       500
#define LAUNCH_REACTIVE_TIMEOUT_S 2

#define LAUNCHER_MIN_WIDTH   790
#define LAUNCHER_MIN_HEIGHT  400

#define LEFT_COLUMN_WIDTH    240.0

#define FILTER_WIDTH 270.0

#define SCROLLVIEW_RESERVED_WIDTH 10
#define SCROLLBAR_RESERVED_WIDTH  40
#define SCROLLVIEW_ROW_SIZE       50.0
#define EXPANDER_GRID_ROW_GAP      8

#define LAUNCHER_GRID_COLUMN_GAP   6
#define LAUNCHER_GRID_ROW_GAP      6
#define LAUNCHER_BUTTON_WIDTH     210
#define LAUNCHER_BUTTON_HEIGHT     79
#define LAUNCHER_BUTTON_ICON_SIZE  48

#define SCROLLVIEW_OUTER_WIDTH(self_)                                          \
          (clutter_actor_get_width (CLUTTER_ACTOR (self_)) -                   \
           SCROLLVIEW_RESERVED_WIDTH)
#define SCROLLVIEW_OUTER_HEIGHT(self_)                                         \
          (clutter_actor_get_height (CLUTTER_ACTOR (self_)) -                  \
           clutter_actor_get_height (self_->priv->filter_hbox) - 35)
#define SCROLLVIEW_INNER_WIDTH(self_) 695.0

#define REAL_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_LAUNCHER, MnbLauncherPrivate))

#ifdef G_DISABLE_CHECKS
  #define GET_PRIVATE(obj) \
          (((MnbLauncher *) obj)->priv)
#else
  #define GET_PRIVATE(obj) \
          REAL_GET_PRIVATE(obj)
#endif /* G_DISABLE_CHECKS */

enum
{
  PROP_0,
};

enum
{
  LAUNCHER_ACTIVATED,

  LAST_SIGNAL
};

static guint _signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (MnbLauncher, mnb_launcher, MX_TYPE_BOX_LAYOUT);

/*
 * Helper struct that contains all the info needed to switch between
 * browser- and filter-mode.
 */
struct MnbLauncherPrivate_ {
  GtkIconTheme            *theme;
  MplAppBookmarkManager   *manager;
  MnbLauncherMonitor      *monitor;
  GHashTable              *expanders;
  MxExpander              *first_expander;
  GSList                  *launchers;

  /* Static widgets, managed by clutter. */
  ClutterActor            *filter_hbox;
  ClutterActor            *filter;
  ClutterActor            *scrollview;
  MxExpander              *default_expander;

  /* "Dynamic" widgets (browser vs. filter mode).
   * These are explicitely ref'd and destroyed. */
  ClutterActor            *fav_grid;
  ClutterActor            *apps_grid;

  /* While filtering. */
  gboolean                 is_filtering;
  guint                    timeout_id;
  char                    *lcase_needle;

  /* Keyboard navigation. */
  guint                    expand_timeout_id;
  MxExpander              *expand_expander;

  /* During incremental fill. */
  guint                    fill_id;
  MnbLauncherTree         *tree;
  GList                   *directories;
  GList const             *directory_iter;
};

static void mnb_launcher_monitor_cb        (MnbLauncherMonitor *monitor,
                                             MnbLauncher        *self);

static void
entry_set_focus (MnbLauncher *self,
                 gboolean     focus)
{
  MnbLauncherPrivate  *priv = GET_PRIVATE (self);

  if (focus)
    clutter_actor_grab_key_focus (priv->filter);
}

static gboolean
launcher_button_set_reactive_cb (ClutterActor *launcher)
{
  clutter_actor_set_reactive (launcher, TRUE);
  return FALSE;
}

static void
launcher_button_hovered_cb (MnbLauncherButton  *launcher,
                            MnbLauncher        *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  MxWidget *expander;

  if (priv->is_filtering)
    {
      const GSList *launchers_iter;
      for (launchers_iter = priv->launchers;
           launchers_iter;
           launchers_iter = launchers_iter->next)
        {
          mx_stylable_set_style_pseudo_class (MX_STYLABLE (launchers_iter->data),
                                              NULL);
        }
    }
  else
    {
      clutter_container_foreach (CLUTTER_CONTAINER (priv->fav_grid),
                                 (ClutterCallback) mx_stylable_set_style_pseudo_class,
                                 NULL);

      expander = mnb_launcher_grid_find_widget_by_pseudo_class (
                  MNB_LAUNCHER_GRID (priv->apps_grid),
                  "active");
      if (expander)
        {
          ClutterActor *inner_grid = mx_bin_get_child (MX_BIN (expander));
          clutter_container_foreach (CLUTTER_CONTAINER (inner_grid),
                                     (ClutterCallback) mx_stylable_set_style_pseudo_class,
                                     NULL);
        }
    }
}

static void
launcher_button_activated_cb (MnbLauncherButton  *launcher,
                              MnbLauncher        *self)
{
  const gchar *desktop_file_path;

  /* Disable button for some time to avoid launching multiple times. */
  clutter_actor_set_reactive (CLUTTER_ACTOR (launcher), FALSE);
  g_timeout_add_seconds (LAUNCH_REACTIVE_TIMEOUT_S,
                         (GSourceFunc) launcher_button_set_reactive_cb,
                         launcher);

  desktop_file_path = mnb_launcher_button_get_desktop_file_path (launcher);

  g_signal_emit (self, _signals[LAUNCHER_ACTIVATED], 0, desktop_file_path);
}

static void
launcher_button_fav_toggled_cb (MnbLauncherButton  *launcher,
                                MnbLauncher        *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  gchar   *uri = NULL;
  GError  *error = NULL;

  if (mnb_launcher_button_get_favorite (launcher))
    {
      MxWidget *clone = mnb_launcher_button_create_favorite (launcher);
      clutter_container_add (CLUTTER_CONTAINER (priv->fav_grid),
                             CLUTTER_ACTOR (clone), NULL);
      g_signal_connect (clone, "hovered",
                        G_CALLBACK (launcher_button_hovered_cb),
                        self);
      g_signal_connect (clone, "activated",
                        G_CALLBACK (launcher_button_activated_cb),
                        self);

      /* Update bookmarks. */
      uri = g_strdup_printf ("file://%s",
              mnb_launcher_button_get_desktop_file_path (
                MNB_LAUNCHER_BUTTON (clone)));
      mpl_app_bookmark_manager_add_uri (priv->manager,
                                        uri);
    }
  else
    {
      /* Update bookmarks. */
      uri = g_strdup_printf ("file://%s",
              mnb_launcher_button_get_desktop_file_path (
                MNB_LAUNCHER_BUTTON (launcher)));
      mpl_app_bookmark_manager_remove_uri (priv->manager,
                                           uri);
    }

  if (error)
    {
      g_warning ("%s", error->message);
      g_clear_error (&error);
    }

  g_free (uri);
  mpl_app_bookmark_manager_save (priv->manager);
}

static void
launcher_button_reload_icon_cb (ClutterActor  *launcher,
                                GtkIconTheme  *theme)
{
  if (!MNB_IS_LAUNCHER_BUTTON (launcher))
    return;

  const gchar *icon_name = mnb_launcher_button_get_icon_name (MNB_LAUNCHER_BUTTON (launcher));
  gchar *icon_file = mpl_icon_theme_lookup_icon_file (theme, icon_name, LAUNCHER_BUTTON_ICON_SIZE);
  mnb_launcher_button_set_icon (MNB_LAUNCHER_BUTTON (launcher), icon_file, LAUNCHER_BUTTON_ICON_SIZE);
  g_free (icon_file);

}

static MxWidget *
launcher_button_create_from_entry (MnbLauncherApplication *entry,
                                   const gchar      *category,
                                   GtkIconTheme     *theme)
{
  const gchar *generic_name, *description, *exec, *icon_name;
  gchar *icon_file;
  MxWidget  *button;

  description = NULL;
  exec = NULL;
  icon_name = NULL;
  button = NULL;

  generic_name = mnb_launcher_application_get_name (entry);
  exec = mnb_launcher_application_get_executable (entry);
  description = mnb_launcher_application_get_description (entry);
  icon_name = mnb_launcher_application_get_icon (entry);
  icon_file = mpl_icon_theme_lookup_icon_file (theme, icon_name, LAUNCHER_BUTTON_ICON_SIZE);

  if (generic_name && exec && icon_file)
    {
      button = mnb_launcher_button_new (icon_name, icon_file, LAUNCHER_BUTTON_ICON_SIZE,
                                        generic_name, category,
                                        description, exec,
                                        mnb_launcher_application_get_desktop_file (entry));
      clutter_actor_set_size (CLUTTER_ACTOR (button),
                              LAUNCHER_BUTTON_WIDTH,
                              LAUNCHER_BUTTON_HEIGHT);
    }

  g_free (icon_file);

  return button;
}

static void
expander_expand_complete_cb (MxExpander       *expander,
                             MnbLauncher      *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  /* Cancel keyboard navigation to not interfere with the mouse. */
  if (priv->expand_timeout_id)
    {
      g_source_remove (priv->expand_timeout_id);
      priv->expand_timeout_id = 0;
      priv->expand_expander = NULL;
    }

  if (mx_expander_get_expanded (expander))
    {
      priv->expand_expander = expander;

      /* Do not highlight if the focus has already moved on to fav apps. */
      ClutterActor *inner_grid = mx_bin_get_child (MX_BIN (priv->expand_expander));
      ClutterActor *launcher = (ClutterActor *) mnb_launcher_grid_find_widget_by_pseudo_class (
                                                  MNB_LAUNCHER_GRID (inner_grid),
                                                  "hover");

      if (!launcher)
        mnb_launcher_grid_keynav_first (MNB_LAUNCHER_GRID (inner_grid));

      /* FIXME Interestingly mx_scroll_view_ensure_visible() on the expander
       * here totally messes up. */
    }
  else
    {
      ClutterActor *inner_grid;
      inner_grid = mx_bin_get_child (MX_BIN (expander));
      mnb_launcher_grid_keynav_out (MNB_LAUNCHER_GRID (inner_grid));
    }
}

static void
expander_expanded_notify_cb (MxExpander      *expander,
                             GParamSpec      *pspec,
                             MnbLauncher     *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  MxExpander      *e;
  const gchar     *category;
  GHashTableIter   iter;

  /* Cancel keyboard navigation to not interfere with the mouse. */
  if (priv->expand_timeout_id)
    {
      g_source_remove (priv->expand_timeout_id);
      priv->expand_timeout_id = 0;
      priv->expand_expander = NULL;
    }

  /* Close other open expander, so that just the newly opended one is expanded. */
  if (mx_expander_get_expanded (expander))
    {
      g_hash_table_iter_init (&iter, priv->expanders);
      while (g_hash_table_iter_next (&iter,
                                     (gpointer *) &category,
                                     (gpointer *) &e))
        {
          if (e != expander)
            {
              ClutterActor *inner_grid = mx_bin_get_child (MX_BIN (e));
              mnb_launcher_grid_keynav_out (MNB_LAUNCHER_GRID (inner_grid));
              mx_expander_set_expanded (e, FALSE);
            }
        }
    }
  else
    {
      ClutterActor *inner_grid = mx_bin_get_child (MX_BIN (expander));
      mnb_launcher_grid_keynav_out (MNB_LAUNCHER_GRID (inner_grid));
    }
}

static void
expander_frame_allocated_cb (MxExpander             *expander,
                             ClutterActorBox const  *box,
                             MnbLauncher            *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  ClutterGeometry geometry;

  geometry.x = box->x1;
  geometry.y = box->y1;
  geometry.width = box->x2 - box->x1;
  geometry.height = box->y2 - box->y1;

  mx_scroll_view_ensure_visible (MX_SCROLL_VIEW (priv->scrollview), &geometry);
}

static void
mnb_launcher_cancel_search (MnbLauncher     *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  /* Abort current search if any. */
  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
      g_free (priv->lcase_needle);
      priv->lcase_needle = NULL;
    }
}

static void
mnb_launcher_reset (MnbLauncher     *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  mnb_launcher_cancel_search (self);

  /* Clear fav apps */
  clutter_container_foreach (CLUTTER_CONTAINER (priv->fav_grid),
                             (ClutterCallback) clutter_actor_destroy,
                             NULL);

  /* Clear apps */
  clutter_actor_destroy (priv->apps_grid);
  priv->apps_grid = NULL;

  g_hash_table_destroy (priv->expanders);
  priv->expanders = NULL;

  g_slist_free (priv->launchers);
  priv->launchers = NULL;

  /* Shut down monitoring */
  if (priv->monitor)
    {
      mnb_launcher_monitor_free (priv->monitor);
      priv->monitor = NULL;
    }
}

static gboolean
mnb_launcher_fill_category (MnbLauncher     *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  MnbLauncherDirectory  *directory;
  GList                 *entry_iter;
  ClutterActor          *inner_grid;
  MxWidget              *button;
  int                    n_buttons = 0;

  if (priv->tree == NULL)
    {
      /* First invocation. */
      priv->tree = mnb_launcher_tree_create ();
      priv->directories = mnb_launcher_tree_list_entries (priv->tree);
      priv->directory_iter = priv->directories;
    }
  else
    {
      /* N-th invocation. */
      priv->directory_iter = priv->directory_iter->next;
    }

  if (priv->directory_iter == NULL)
    {
      /* Last invocation. */

      /* Expand default expander */
      if (priv->default_expander)
        mx_expander_set_expanded (priv->default_expander, TRUE);
      else
        g_warning ("%s : No default expander 'Internet'", G_STRLOC);

      /* Alphabetically sort buttons, so they are in order while filtering. */
      priv->launchers = g_slist_sort (priv->launchers,
                                      (GCompareFunc) mnb_launcher_button_compare);

      /* Create monitor only once. */
      if (!priv->monitor)
        {
          priv->monitor =
            mnb_launcher_tree_create_monitor (
              priv->tree,
              (MnbLauncherMonitorFunction) mnb_launcher_monitor_cb,
               self);
        }

      priv->fill_id = 0;
      mnb_launcher_tree_free_entries (priv->directories);
      priv->directories = NULL;
      priv->directory_iter = NULL;
      mnb_launcher_tree_free (priv->tree);
      priv->tree = NULL;

      return FALSE;
    }

  /* Create and fill one category. */

  directory = (MnbLauncherDirectory *) priv->directory_iter->data;

  inner_grid = CLUTTER_ACTOR (mnb_launcher_grid_new ());
  mx_grid_set_column_spacing (MX_GRID (inner_grid), LAUNCHER_GRID_COLUMN_GAP);
  mx_grid_set_row_spacing (MX_GRID (inner_grid), LAUNCHER_GRID_ROW_GAP);
  clutter_actor_set_name (inner_grid, "launcher-expander-grid");

  button = NULL;
  for (entry_iter = directory->entries; entry_iter; entry_iter = entry_iter->next)
    {
      button = launcher_button_create_from_entry ((MnbLauncherApplication *) entry_iter->data,
                                                  directory->name,
                                                  priv->theme);
      if (button)
        {
          /* Assuming limited number of fav apps, linear search should do for now. */
          if (priv->fav_grid)
            {
              clutter_container_foreach (CLUTTER_CONTAINER (priv->fav_grid),
                                          (ClutterCallback) mnb_launcher_button_sync_if_favorite,
                                          button);
            }

          clutter_container_add (CLUTTER_CONTAINER (inner_grid),
                                  CLUTTER_ACTOR (button), NULL);
          g_signal_connect (button, "hovered",
                            G_CALLBACK (launcher_button_hovered_cb),
                            self);
          g_signal_connect (button, "activated",
                            G_CALLBACK (launcher_button_activated_cb),
                            self);
          g_signal_connect (button, "fav-toggled",
                            G_CALLBACK (launcher_button_fav_toggled_cb),
                            self);
          priv->launchers = g_slist_prepend (priv->launchers,
                                                      button);
          n_buttons++;
        }
    }

    /* Create expander if at least 1 launcher inside. */
    if (n_buttons > 0)
      {
        ClutterActor *expander;

        expander = CLUTTER_ACTOR (mnb_expander_new ());
        mx_expander_set_label (MX_EXPANDER (expander),
                                  directory->name);
        clutter_actor_set_width (expander, SCROLLVIEW_INNER_WIDTH (self));
        clutter_container_add_actor (CLUTTER_CONTAINER (priv->apps_grid),
                                     expander);
        g_hash_table_insert (priv->expanders,
                              g_strdup (directory->name), expander);
        clutter_container_add (CLUTTER_CONTAINER (expander), inner_grid, NULL);

        /* This expander will be opened by default. */
        if (0 == g_strcmp0 ("Internet", directory->id))
          {
            priv->default_expander = MX_EXPANDER (expander);
            g_object_add_weak_pointer ((GObject *) priv->default_expander,
                                       (gpointer *) &priv->default_expander);
          }

        g_signal_connect (expander, "notify::expanded",
                          G_CALLBACK (expander_expanded_notify_cb),
                          self);
        g_signal_connect (expander, "expand-complete",
                          G_CALLBACK (expander_expand_complete_cb),
                          self);
        g_signal_connect (expander, "frame-allocated",
                          G_CALLBACK (expander_frame_allocated_cb),
                          self);
      }
    else
      {
        clutter_actor_destroy (inner_grid);
      }

  return TRUE;
}

static void
mnb_launcher_fill (MnbLauncher     *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  GList *fav_apps;
  gboolean have_fav_apps;

  /*
   * Fav apps.
   */

  have_fav_apps = FALSE;
  fav_apps = mpl_app_bookmark_manager_get_bookmarks (priv->manager);
  if (fav_apps)
    {
      GList *fav_apps_iter;

      for (fav_apps_iter = fav_apps;
           fav_apps_iter;
           fav_apps_iter = fav_apps_iter->next)
        {
          gchar             *uri;
          gchar             *desktop_file_path;
          MnbLauncherApplication  *entry;
          MxWidget          *button = NULL;
          GError            *error = NULL;

          uri = (gchar *) fav_apps_iter->data;
          desktop_file_path = g_filename_from_uri (uri, NULL, &error);
          if (error)
            {
              g_warning ("%s", error->message);
              g_clear_error (&error);
              continue;
            }

          entry = mnb_launcher_application_new_from_desktop_file (desktop_file_path);
          g_free (desktop_file_path);
          if (entry)
            {
              button = launcher_button_create_from_entry (entry, NULL, priv->theme);
              g_object_unref (entry);
            }

          if (button)
            {
              have_fav_apps = TRUE;
              mnb_launcher_button_set_favorite (MNB_LAUNCHER_BUTTON (button),
                                                TRUE);
              clutter_container_add (CLUTTER_CONTAINER (priv->fav_grid),
                                     CLUTTER_ACTOR (button), NULL);
              g_signal_connect (button, "hovered",
                                G_CALLBACK (launcher_button_hovered_cb),
                                self);
              g_signal_connect (button, "activated",
                                G_CALLBACK (launcher_button_activated_cb),
                                self);
              g_signal_connect (button, "fav-toggled",
                                G_CALLBACK (launcher_button_fav_toggled_cb),
                                self);
            }
        }
      g_list_free (fav_apps);
    }

  /*
   * Apps browser.
   */

  /* Grid */
  priv->apps_grid = CLUTTER_ACTOR (mnb_launcher_grid_new ());
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->scrollview),
                                priv->apps_grid);
  mx_grid_set_row_spacing (MX_GRID (priv->apps_grid),
                         EXPANDER_GRID_ROW_GAP);

  priv->expanders = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                    g_free, NULL);

  priv->fill_id = g_idle_add ((GSourceFunc) mnb_launcher_fill_category,
                              self);
}

static void
mnb_launcher_theme_changed_cb (GtkIconTheme    *theme,
                                MnbLauncher     *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  clutter_container_foreach (CLUTTER_CONTAINER (priv->fav_grid),
                             (ClutterCallback) launcher_button_reload_icon_cb,
                             priv->theme);

  g_slist_foreach (priv->launchers,
                   (GFunc) launcher_button_reload_icon_cb,
                   priv->theme);
}

static void
mnb_launcher_monitor_cb (MnbLauncherMonitor  *monitor,
                          MnbLauncher         *self)
{
  mnb_launcher_reset (self);
  mnb_launcher_fill (self);
}

static gboolean
mnb_launcher_filter_cb (MnbLauncher *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  GSList *iter;

  if (priv->lcase_needle)
    {
      /* Need to switch to filter mode? */
      if (!priv->is_filtering)
        {
          GSList          *iter;
          GHashTableIter   expander_iter;
          ClutterActor    *expander;

          priv->is_filtering = TRUE;

          mx_grid_set_row_spacing (MX_GRID (priv->apps_grid),
                                 LAUNCHER_GRID_ROW_GAP);
          mx_grid_set_column_spacing (MX_GRID (priv->apps_grid),
                                    LAUNCHER_GRID_COLUMN_GAP);

          /* Hide expanders. */
          g_hash_table_iter_init (&expander_iter, priv->expanders);
          while (g_hash_table_iter_next (&expander_iter,
                                          NULL,
                                          (gpointer *) &expander))
            {
              clutter_actor_hide (expander);
            }

          /* Reparent launchers onto grid.
            * Launchers are initially invisible to avoid bogus matches. */
          for (iter = priv->launchers; iter; iter = iter->next)
            {
              MnbLauncherButton *launcher = MNB_LAUNCHER_BUTTON (iter->data);
              clutter_actor_hide (CLUTTER_ACTOR (launcher));
              clutter_actor_reparent (CLUTTER_ACTOR (launcher),
                                      priv->apps_grid);
              mx_stylable_set_style_pseudo_class (MX_STYLABLE (launcher), NULL);
            }
        }

      /* Perform search. */
      for (iter = priv->launchers; iter; iter = iter->next)
        {
          MnbLauncherButton *button = MNB_LAUNCHER_BUTTON (iter->data);
          if (mnb_launcher_button_match (button, priv->lcase_needle))
            {
              clutter_actor_show (CLUTTER_ACTOR (button));
            }
          else
            {
              clutter_actor_hide (CLUTTER_ACTOR (button));
              mx_stylable_set_style_pseudo_class (MX_STYLABLE (button), NULL);
            }
        }

      g_free (priv->lcase_needle);
      priv->lcase_needle = NULL;
    }
  else if (priv->is_filtering)
    {
      /* Did filter, now switch back to normal mode */
      GHashTableIter   expander_iter;
      ClutterActor    *expander;

      priv->is_filtering = FALSE;

      mx_grid_set_row_spacing (MX_GRID (priv->apps_grid),
                               EXPANDER_GRID_ROW_GAP);
      mx_grid_set_column_spacing (MX_GRID (priv->apps_grid), 0);

      /* Reparent launchers into expanders. */
      for (iter = priv->launchers; iter; iter = iter->next)
        {
          MnbLauncherButton *launcher   = MNB_LAUNCHER_BUTTON (iter->data);
          const gchar       *category   = mnb_launcher_button_get_category (launcher);
          ClutterActor      *e          = g_hash_table_lookup (priv->expanders, category);
          ClutterActor      *inner_grid = mx_bin_get_child (MX_BIN (e));

          mx_stylable_set_style_pseudo_class (MX_STYLABLE (launcher), NULL);
          clutter_actor_reparent (CLUTTER_ACTOR (launcher), inner_grid);
        }

      /* Show expanders. */
      g_hash_table_iter_init (&expander_iter, priv->expanders);
      while (g_hash_table_iter_next (&expander_iter, NULL, (gpointer *) &expander))
        {
          clutter_actor_show (expander);
        }
    }

  clutter_actor_queue_relayout (priv->apps_grid);
  return FALSE;
}

static void
_filter_text_notify_cb (MnbFilter     *filter,
                        GParamSpec    *pspec,
                        MnbLauncher   *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  gchar *needle;

  /* Switch back to edit mode. */
  entry_set_focus (self, TRUE);

  mnb_launcher_cancel_search (self);

  needle = g_strdup (mnb_filter_get_text (MNB_FILTER (filter)));
  needle = g_strstrip (needle);

  if (needle && *needle)
    priv->lcase_needle = g_utf8_strdown (needle, -1);
  priv->timeout_id = g_timeout_add (SEARCH_APPLY_TIMEOUT,
                                              (GSourceFunc) mnb_launcher_filter_cb,
                                              self);

  g_free (needle);
}

static void
_dispose (GObject *object)
{
  MnbLauncher *self = MNB_LAUNCHER (object);
  MnbLauncherPrivate *priv = GET_PRIVATE (object);

  if (priv->theme)
    {
      g_signal_handlers_disconnect_by_func (priv->theme, mnb_launcher_theme_changed_cb, object);
      priv->theme = NULL;
    }

  if (priv->manager)
    {
      g_object_unref (priv->manager);
      priv->manager = NULL;
    }

  mnb_launcher_reset (self);

  G_OBJECT_CLASS (mnb_launcher_parent_class)->dispose (object);
}

static void
_key_focus_in (ClutterActor *actor)
{
  entry_set_focus (MNB_LAUNCHER (actor), TRUE);
}

static GObject *
_constructor (GType                  gtype,
              guint                  n_properties,
              GObjectConstructParam *properties)
{
  MnbLauncher *self = (MnbLauncher *) G_OBJECT_CLASS (mnb_launcher_parent_class)
                                        ->constructor (gtype, n_properties, properties);

  MnbLauncherPrivate *priv = self->priv = REAL_GET_PRIVATE (self);
  ClutterActor  *columns;
  ClutterActor  *pane;
  ClutterActor  *label;
  ClutterActor  *fav_scroll;

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);

  /* Panel label */
  label = mx_label_new_with_text (_("Applications"));
  clutter_actor_set_name (label, "panel-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), label);

  columns = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (columns), 12.0);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), columns);
  clutter_container_child_set (CLUTTER_CONTAINER (self), columns,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "y-fill", TRUE,
                               NULL);

  /*
   * Fav apps pane
   */
  pane = mpl_content_pane_new (_("Favorite applications"));
  clutter_actor_set_width (pane, 250.0);
  clutter_container_add_actor (CLUTTER_CONTAINER (columns), pane);

  fav_scroll = mx_scroll_view_new ();
  clutter_actor_set_name (fav_scroll, "fav-pane-content");
  g_object_set (fav_scroll, "clip-to-allocation", TRUE, NULL);
  mx_scroll_view_set_scroll_policy (MX_SCROLL_VIEW (fav_scroll),
                                    MX_SCROLL_POLICY_VERTICAL);
  clutter_container_add_actor (CLUTTER_CONTAINER (pane), fav_scroll);
  clutter_container_child_set (CLUTTER_CONTAINER (pane), fav_scroll,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "y-fill", TRUE,
                               NULL);

  priv->fav_grid = CLUTTER_ACTOR (mnb_launcher_grid_new ());
  mx_stylable_set_style_class (MX_STYLABLE (priv->fav_grid), "fav-grid");
  mx_grid_set_max_stride (MX_GRID (priv->fav_grid), 1);
  clutter_container_add_actor (CLUTTER_CONTAINER (fav_scroll), priv->fav_grid);

  /*
   * Applications
   */

  pane = mpl_content_pane_new (_("Your applications"));
  clutter_container_add_actor (CLUTTER_CONTAINER (columns), pane);
  clutter_container_child_set (CLUTTER_CONTAINER (columns), pane,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "y-fill", TRUE,
                               NULL);

  /* Filter */
  priv->filter = mnb_filter_new ();
  clutter_actor_set_width (priv->filter, FILTER_WIDTH);
  mpl_content_pane_set_header_actor (MPL_CONTENT_PANE (pane), priv->filter);

  /* Apps */
  priv->scrollview = CLUTTER_ACTOR (mx_scroll_view_new ());
  clutter_actor_set_name (priv->scrollview, "apps-pane-content");
  g_object_set (priv->scrollview, "clip-to-allocation", TRUE, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (pane), priv->scrollview);
  clutter_container_child_set (CLUTTER_CONTAINER (pane), priv->scrollview,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "y-fill", TRUE,
                               NULL);

  priv->theme = gtk_icon_theme_get_default ();
  g_signal_connect (priv->theme, "changed",
                    G_CALLBACK (mnb_launcher_theme_changed_cb), self);
  priv->manager = mpl_app_bookmark_manager_get_default ();

  mnb_launcher_fill (self);

  /* Hook up search. */
/*
  g_signal_connect_data (entry, "button-clicked",
                         G_CALLBACK (entry_changed_cb), self,
                         (GClosureNotify) mnb_launcher_free_cb, 0);
*/
  // g_signal_connect (priv->filter, "button-clicked",
  //                   G_CALLBACK (mnb_launcher_theme_changed_cb), self);
  g_signal_connect (priv->filter, "notify::text",
                    G_CALLBACK (_filter_text_notify_cb), self);
  // g_signal_connect (priv->filter, "keynav-event",
  //                   G_CALLBACK (entry_keynav_cb), self);

  return (GObject *) self;
}

static void
_get_property (GObject    *gobject,
               guint       prop_id,
               GValue     *value,
               GParamSpec *pspec)
{
  switch (prop_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
        break;
    }
}

static void
_set_property (GObject      *gobject,
               guint         prop_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  switch (prop_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
        break;
    }
}

static void
mnb_launcher_class_init (MnbLauncherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbLauncherPrivate));

  object_class->constructor = _constructor;
  object_class->dispose = _dispose;
  object_class->set_property = _set_property;
  object_class->get_property = _get_property;

  actor_class->key_focus_in = _key_focus_in;

  /* Properties */


  /* Signals */

  _signals[LAUNCHER_ACTIVATED] =
    g_signal_new ("launcher-activated",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbLauncherClass, launcher_activated),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
mnb_launcher_init (MnbLauncher *self)
{
}

ClutterActor *
mnb_launcher_new (void)
{
  return g_object_new (MNB_TYPE_LAUNCHER,
                       "min-width", (gfloat) LAUNCHER_MIN_WIDTH,
                       "min-height", (gfloat) LAUNCHER_MIN_HEIGHT,
                       NULL);
}

void
mnb_launcher_ensure_filled (MnbLauncher *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  /* Force fill if idle-fill in progress. */
  if (priv->fill_id)
    {
      g_source_remove (priv->fill_id);
      while (mnb_launcher_fill_category (self))
        ;
    }
}

void
mnb_launcher_clear_filter (MnbLauncher *self)
{
  MnbLauncherPrivate  *priv = GET_PRIVATE (self);
  MxAdjustment        *adjust;

  mnb_filter_set_text (MNB_FILTER (priv->filter), "");

  /* Reset scroll position. */
  mx_scrollable_get_adjustments (MX_SCROLLABLE (self->priv->apps_grid),
                                 NULL,
                                 &adjust);
  mx_adjustment_set_value (adjust, 0.0);

  /* Expand default expander. */
  if (priv->default_expander)
    {
      mx_expander_set_expanded (priv->default_expander, TRUE);
    }
}


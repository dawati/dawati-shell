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
#include <nbtk/nbtk.h>
#include <moblin-panel/mpl-icon-theme.h>

#include <moblin-panel/mpl-app-bookmark-manager.h>
#include <moblin-panel/mpl-entry.h>

#include "moblin-netbook-launcher.h"
#include "mnb-expander.h"
#include "mnb-launcher-button.h"
#include "mnb-launcher-grid.h"
#include "mnb-launcher-tree.h"

static void
scrollable_ensure_box_visible (NbtkScrollable         *scrollable,
                               const ClutterActorBox  *box)
{
  NbtkAdjustment *hadjustment;
  NbtkAdjustment *vadjustment;
  gdouble         top, right, bottom, left;
  gdouble         h_page, v_page;

  g_object_get (scrollable,
                "hadjustment", &hadjustment,
                NULL);

  g_object_get (scrollable,
                "vadjustment", &vadjustment,
                NULL);

  g_object_get (hadjustment,
                "value", &left,
                "page-size", &h_page,
                NULL);
  right = left + h_page;

  g_object_get (vadjustment,
                "value", &top,
                "page-size", &v_page,
                NULL);
  bottom = top + v_page;

#define SCROLL_TOP_MARGIN 3

  /* Vertical. */
  if ((box->y1 - SCROLL_TOP_MARGIN) < top)
  {
    nbtk_adjustment_set_value (vadjustment, box->y1 - SCROLL_TOP_MARGIN);
  } else if (box->y2 > bottom) {

    gdouble height = box->y2 - box->y1;
    if (height < v_page)
      nbtk_adjustment_set_value (vadjustment, box->y1 + v_page - height - SCROLL_TOP_MARGIN);
    else
      nbtk_adjustment_set_value (vadjustment, box->y1 - SCROLL_TOP_MARGIN);
  }
}

static void
scrollable_ensure_actor_visible (NbtkScrollable *scrollable,
                                 ClutterActor   *actor)
{
  ClutterActorBox box;
  ClutterVertex   allocation[4];

  clutter_actor_get_allocation_vertices (actor,
                                         CLUTTER_ACTOR (scrollable),
                                         allocation);
  box.x1 = allocation[0].x;
  box.y1 = allocation[0].y;
  box.x2 = allocation[3].x;
  box.y2 = allocation[3].y;

  scrollable_ensure_box_visible (scrollable, &box);
}

static void
container_has_children_cb (ClutterActor  *actor,
                           gboolean      *ret)
{
  *ret = TRUE;
}

static gboolean
container_has_children (ClutterContainer *container)
{
  gboolean ret = FALSE;

  clutter_container_foreach (container,
                             (ClutterCallback) container_has_children_cb,
                             &ret);

  return ret;
}

static void
container_get_n_visible_children_cb (ClutterActor  *actor,
                                     guint         *ret)
{
  if (CLUTTER_ACTOR_IS_VISIBLE (actor))
    *ret += 1;
}

static guint
container_get_n_visible_children (ClutterContainer *container)
{
  guint ret = 0;

  clutter_container_foreach (container,
                             (ClutterCallback) container_get_n_visible_children_cb,
                             &ret);

  return ret;
}

#define SEARCH_APPLY_TIMEOUT       500
#define LAUNCH_REACTIVE_TIMEOUT_S 2

#define LAUNCHER_MIN_WIDTH   790
#define LAUNCHER_MIN_HEIGHT  400

#define FILTER_ENTRY_WIDTH        600

#define SCROLLVIEW_RESERVED_WIDTH 10
#define SCROLLBAR_RESERVED_WIDTH  40
#define SCROLLVIEW_ROW_SIZE       50
#define EXPANDER_GRID_ROW_GAP      8

#define LAUNCHER_GRID_COLUMN_GAP   32
#define LAUNCHER_GRID_ROW_GAP      12
#define LAUNCHER_BUTTON_WIDTH     210
#define LAUNCHER_BUTTON_HEIGHT     79
#define LAUNCHER_BUTTON_ICON_SIZE  48

#define SCROLLVIEW_OUTER_WIDTH(self_)                                          \
          (clutter_actor_get_width (CLUTTER_ACTOR (self_)) -                   \
           SCROLLVIEW_RESERVED_WIDTH)
#define SCROLLVIEW_OUTER_HEIGHT(self_)                                         \
          (clutter_actor_get_height (CLUTTER_ACTOR (self_)) -                  \
           clutter_actor_get_height (self_->priv->filter_hbox) - 35)
#define SCROLLVIEW_INNER_WIDTH(self_)                                          \
          (clutter_actor_get_width (CLUTTER_ACTOR (self_)) -                   \
           SCROLLBAR_RESERVED_WIDTH)

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
  LAUNCHER_ACTIVATED,

  LAST_SIGNAL
};

static guint _signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (MnbLauncher, mnb_launcher, NBTK_TYPE_BIN);

/*
 * Helper struct that contains all the info needed to switch between
 * browser- and filter-mode.
 */
struct MnbLauncherPrivate_ {
  GtkIconTheme            *theme;
  MplAppBookmarkManager   *manager;
  MnbLauncherMonitor      *monitor;
  GHashTable              *expanders;
  NbtkExpander            *first_expander;
  GSList                  *launchers;

  /* Static widgets, managed by clutter. */
  ClutterActor            *filter_hbox;
  ClutterActor            *filter_entry;
  ClutterActor            *scrollview;

  /* "Dynamic" widgets (browser vs. filter mode).
   * These are explicitely ref'd and destroyed. */
  ClutterActor            *scrolled_vbox;
  ClutterActor            *fav_label;
  ClutterActor            *fav_grid;
  ClutterActor            *apps_grid;

  /* While filtering. */
  gboolean                 is_filtering;
  guint                    timeout_id;
  char                    *lcase_needle;

  /* Keyboard navigation. */
  guint                    expand_timeout_id;
  NbtkExpander            *expand_expander;

  /* During incremental fill. */
  guint                    fill_id;
  MnbLauncherTree         *tree;
  GList                   *directories;
  GList const             *directory_iter;
};

static void mnb_launcher_monitor_cb        (MnbLauncherMonitor *monitor,
                                             MnbLauncher        *self);

static void mnb_launcher_set_show_fav_apps (MnbLauncher        *self,
                                             gboolean            show);

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
  NbtkWidget *expander;

  if (priv->is_filtering)
    {
      const GSList *launchers_iter;
      for (launchers_iter = priv->launchers;
           launchers_iter;
           launchers_iter = launchers_iter->next)
        {
          nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (launchers_iter->data),
                                              NULL);
        }
    }
  else
    {
      clutter_container_foreach (CLUTTER_CONTAINER (priv->fav_grid),
                                 (ClutterCallback) nbtk_widget_set_style_pseudo_class,
                                 NULL);

      expander = mnb_launcher_grid_find_widget_by_pseudo_class (
                  MNB_LAUNCHER_GRID (priv->apps_grid),
                  "active");
      if (expander)
        {
          ClutterActor *inner_grid = nbtk_bin_get_child (NBTK_BIN (expander));
          clutter_container_foreach (CLUTTER_CONTAINER (inner_grid),
                                     (ClutterCallback) nbtk_widget_set_style_pseudo_class,
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
      NbtkWidget *clone = mnb_launcher_button_create_favorite (launcher);
      clutter_container_add (CLUTTER_CONTAINER (priv->fav_grid),
                             CLUTTER_ACTOR (clone), NULL);
      g_signal_connect (clone, "hovered",
                        G_CALLBACK (launcher_button_hovered_cb),
                        self);
      g_signal_connect (clone, "activated",
                        G_CALLBACK (launcher_button_activated_cb),
                        self);

      /* Make sure fav apps show up. */
      if (!priv->is_filtering)
        mnb_launcher_set_show_fav_apps (self, TRUE);

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

      /* Hide fav apps after last one removed. */
      if (!container_has_children (CLUTTER_CONTAINER (priv->fav_grid)))
        {
          mnb_launcher_set_show_fav_apps (self, FALSE);
        }
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

static NbtkWidget *
launcher_button_create_from_entry (MnbLauncherApplication *entry,
                                   const gchar      *category,
                                   GtkIconTheme     *theme)
{
  const gchar *generic_name, *description, *exec, *icon_name;
  gchar *icon_file;
  NbtkWidget  *button;

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

static gboolean
expander_expand_complete_idle_cb (MnbLauncher *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  ClutterActor *launcher;

  /* Do not highlight if the focus has already moved on to fav apps. */
  launcher = (ClutterActor *) mnb_launcher_grid_find_widget_by_pseudo_class (
                                MNB_LAUNCHER_GRID (priv->fav_grid),
                                "hover");
  if (launcher)
    return FALSE;

  if (nbtk_expander_get_expanded (priv->expand_expander))
    {
      ClutterActor *inner_grid;

      inner_grid = nbtk_bin_get_child (NBTK_BIN (priv->expand_expander));
      launcher = (ClutterActor *) mnb_launcher_grid_find_widget_by_pseudo_class (
                                    MNB_LAUNCHER_GRID (inner_grid),
                                    "hover");

      if (!launcher)
        launcher = (ClutterActor *) mnb_launcher_grid_keynav_first (MNB_LAUNCHER_GRID (inner_grid));

      priv->expand_timeout_id = 0;
      priv->expand_expander = NULL;
    }

  return FALSE;
}

static void
expander_expand_complete_cb (NbtkExpander     *expander,
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

  if (nbtk_expander_get_expanded (expander))
    {
      priv->expand_expander = expander;
      priv->expand_timeout_id = g_idle_add ((GSourceFunc) expander_expand_complete_idle_cb,
                                            self);
      scrollable_ensure_actor_visible (NBTK_SCROLLABLE (priv->scrolled_vbox),
                                       CLUTTER_ACTOR (expander));
    }
  else
    {
      ClutterActor *inner_grid;
      inner_grid = nbtk_bin_get_child (NBTK_BIN (expander));
      mnb_launcher_grid_keynav_out (MNB_LAUNCHER_GRID (inner_grid));
    }
}

static void
expander_expanded_notify_cb (NbtkExpander    *expander,
                             GParamSpec      *pspec,
                             MnbLauncher     *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  NbtkExpander    *e;
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
  if (nbtk_expander_get_expanded (expander))
    {
      g_hash_table_iter_init (&iter, priv->expanders);
      while (g_hash_table_iter_next (&iter,
                                     (gpointer *) &category,
                                     (gpointer *) &e))
        {
          if (e != expander)
            {
              ClutterActor *inner_grid = nbtk_bin_get_child (NBTK_BIN (e));
              mnb_launcher_grid_keynav_out (MNB_LAUNCHER_GRID (inner_grid));
              nbtk_expander_set_expanded (e, FALSE);
            }
        }
    }
  else
    {
      ClutterActor *inner_grid = nbtk_bin_get_child (NBTK_BIN (expander));
      mnb_launcher_grid_keynav_out (MNB_LAUNCHER_GRID (inner_grid));
    }
}

static void
expander_frame_allocated_cb (NbtkExpander           *expander,
                             ClutterActorBox const  *box,
                             MnbLauncher            *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  scrollable_ensure_actor_visible (NBTK_SCROLLABLE (priv->scrolled_vbox),
                                   CLUTTER_ACTOR (expander));
}

static gboolean
expander_expand_cb (MnbLauncher     *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  priv->expand_timeout_id = 0;
  nbtk_expander_set_expanded (priv->expand_expander, TRUE);
  priv->expand_expander = NULL;

  return FALSE;
}

static void
mnb_launcher_hover_expander (MnbLauncher     *self,
                              NbtkExpander    *expander)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  if (priv->expand_timeout_id)
    {
      g_source_remove (priv->expand_timeout_id);
      priv->expand_timeout_id = 0;
      priv->expand_expander = NULL;
    }

  if (expander)
    {
      nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (expander), "hover");
      priv->expand_expander = expander;
      priv->expand_timeout_id = g_timeout_add (SEARCH_APPLY_TIMEOUT,
                                                        (GSourceFunc) expander_expand_cb,
                                                        self);
      scrollable_ensure_actor_visible (NBTK_SCROLLABLE (priv->scrolled_vbox),
                                       CLUTTER_ACTOR (expander));
    }
}

static gboolean
mnb_launcher_keynav_in_grid (MnbLauncher       *self,
                              MnbLauncherGrid   *grid,
                              guint              keyval)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  NbtkWidget *launcher;

  launcher = mnb_launcher_grid_find_widget_by_pseudo_class (grid, "hover");
  if (launcher)
    {
      /* Do the navigation. */
      launcher = mnb_launcher_grid_keynav (grid, keyval);
      if (launcher && MNB_IS_LAUNCHER_BUTTON (launcher))
        {
          if (keyval == CLUTTER_Return)
            {
              launcher_button_activated_cb (MNB_LAUNCHER_BUTTON (launcher),
                                            self);
            }
          else
            {
              scrollable_ensure_actor_visible (NBTK_SCROLLABLE (priv->scrolled_vbox),
                                               CLUTTER_ACTOR (launcher));
            }

            return TRUE;
        }
    }
  else
    {
      /* Nothing focused, jump to first actor. */
      launcher = mnb_launcher_grid_keynav_first (grid);
      if (launcher)
        {
          /* Activate if it is the only one. */
          if (container_get_n_visible_children (CLUTTER_CONTAINER (grid)) == 1 &&
              keyval == CLUTTER_Return)
            {
              launcher_button_activated_cb (MNB_LAUNCHER_BUTTON (launcher),
                                            self);
            } else {
              scrollable_ensure_actor_visible (NBTK_SCROLLABLE (priv->scrolled_vbox),
                                               CLUTTER_ACTOR (launcher));
            }
          return TRUE;
        }
    }

  return FALSE;
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

  clutter_actor_destroy (priv->scrolled_vbox);
  priv->scrolled_vbox = NULL;

  g_object_unref (priv->fav_label);
  priv->fav_label = NULL;

  g_object_unref (priv->fav_grid);
  priv->fav_grid = NULL;

  g_hash_table_destroy (priv->expanders);
  priv->expanders = NULL;

  g_slist_free (priv->launchers);
  priv->launchers = NULL;

  if (priv->monitor)
    {
      mnb_launcher_monitor_free (priv->monitor);
      priv->monitor = NULL;
    }
}

static void
mnb_launcher_set_show_fav_apps (MnbLauncher     *self,
                                 gboolean         show)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  if (show)
    {
      clutter_actor_show (priv->fav_label);
      clutter_actor_show (priv->fav_grid);
    }
  else
    {
      clutter_actor_hide (priv->fav_label);
      clutter_actor_hide (priv->fav_grid);
    }
}

static gboolean
mnb_launcher_fill_category (MnbLauncher     *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  MnbLauncherDirectory  *directory;
  GList                 *entry_iter;
  ClutterActor          *inner_grid;
  NbtkWidget            *button;
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
  nbtk_grid_set_column_gap (NBTK_GRID (inner_grid), LAUNCHER_GRID_COLUMN_GAP);
  nbtk_grid_set_row_gap (NBTK_GRID (inner_grid), LAUNCHER_GRID_ROW_GAP);
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
        nbtk_expander_set_label (NBTK_EXPANDER (expander),
                                  directory->name);
        clutter_actor_set_width (expander, SCROLLVIEW_INNER_WIDTH (self));
        clutter_container_add (CLUTTER_CONTAINER (priv->apps_grid),
                                expander, NULL);
        g_hash_table_insert (priv->expanders,
                              g_strdup (directory->name), expander);
        clutter_container_add (CLUTTER_CONTAINER (expander), inner_grid, NULL);

        /* Remember and open first expander by default. */
        if (priv->directory_iter == priv->directories)
        {
          nbtk_expander_set_expanded (NBTK_EXPANDER (expander), TRUE);
          priv->first_expander = NBTK_EXPANDER (expander);
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

  if (priv->scrolled_vbox == NULL)
    {
      priv->scrolled_vbox = CLUTTER_ACTOR (mnb_launcher_grid_new ());
      g_object_set (priv->scrolled_vbox,
                    "max-stride", 1,
                    NULL);
      clutter_container_add (CLUTTER_CONTAINER (priv->scrollview),
                             priv->scrolled_vbox, NULL);
    }

  /*
   * Fav apps.
   */

  /* Label */
  priv->fav_label = CLUTTER_ACTOR (nbtk_label_new (_("Favorite Applications")));
  clutter_container_add (CLUTTER_CONTAINER (priv->scrolled_vbox),
                         priv->fav_label, NULL);
  g_object_ref (priv->fav_label);
  clutter_actor_set_name (priv->fav_label, "launcher-group-label");

  /* Grid */
  priv->fav_grid = CLUTTER_ACTOR (mnb_launcher_grid_new ());
  clutter_container_add (CLUTTER_CONTAINER (priv->scrolled_vbox),
                         priv->fav_grid, NULL);
  nbtk_grid_set_row_gap (NBTK_GRID (priv->fav_grid), LAUNCHER_GRID_ROW_GAP);
  nbtk_grid_set_column_gap (NBTK_GRID (priv->fav_grid), LAUNCHER_GRID_COLUMN_GAP);
  clutter_actor_set_width (priv->fav_grid, SCROLLVIEW_INNER_WIDTH (self));
  clutter_actor_set_name (priv->fav_grid, "launcher-fav-grid");
  g_object_ref (priv->fav_grid);

  have_fav_apps = FALSE;
  fav_apps = mpl_app_bookmark_manager_get_bookmarks (priv->manager);
  if (fav_apps)
    {
      GList *fav_apps_iter;

      mnb_launcher_set_show_fav_apps (self, TRUE);

      for (fav_apps_iter = fav_apps;
           fav_apps_iter;
           fav_apps_iter = fav_apps_iter->next)
        {
          gchar             *uri;
          gchar             *desktop_file_path;
          MnbLauncherApplication  *entry;
          NbtkWidget        *button = NULL;
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

  if (!have_fav_apps)
    mnb_launcher_set_show_fav_apps (self, FALSE);

  /*
   * Apps browser.
   */

  /* Grid */
  priv->apps_grid = CLUTTER_ACTOR (mnb_launcher_grid_new ());
  clutter_container_add (CLUTTER_CONTAINER (priv->scrolled_vbox),
                         priv->apps_grid, NULL);
  clutter_actor_set_name (priv->apps_grid, "launcher-apps-grid");
  clutter_actor_set_width (priv->apps_grid, SCROLLVIEW_INNER_WIDTH (self));
  nbtk_grid_set_row_gap (NBTK_GRID (priv->apps_grid),
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
          mnb_launcher_set_show_fav_apps (self, FALSE);

          nbtk_grid_set_row_gap (NBTK_GRID (priv->apps_grid),
                                 LAUNCHER_GRID_ROW_GAP);
          nbtk_grid_set_column_gap (NBTK_GRID (priv->apps_grid),
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
              nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (launcher), NULL);
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
              nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (button), NULL);
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

      nbtk_grid_set_row_gap (NBTK_GRID (priv->apps_grid),
                             EXPANDER_GRID_ROW_GAP);
      nbtk_grid_set_column_gap (NBTK_GRID (priv->apps_grid), 0);

      if (container_has_children (CLUTTER_CONTAINER (priv->fav_grid)))
        mnb_launcher_set_show_fav_apps (self, TRUE);

      /* Reparent launchers into expanders. */
      for (iter = priv->launchers; iter; iter = iter->next)
        {
          MnbLauncherButton *launcher   = MNB_LAUNCHER_BUTTON (iter->data);
          const gchar       *category   = mnb_launcher_button_get_category (launcher);
          ClutterActor      *e          = g_hash_table_lookup (priv->expanders, category);
          ClutterActor      *inner_grid = nbtk_bin_get_child (NBTK_BIN (e));

          nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (launcher), NULL);
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
entry_changed_cb (MplEntry         *entry,
                  MnbLauncher      *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  gchar *needle;

  mnb_launcher_cancel_search (self);

  needle = g_strdup (mpl_entry_get_text (entry));
  needle = g_strstrip (needle);

  if (needle && *needle)
    priv->lcase_needle = g_utf8_strdown (needle, -1);
  priv->timeout_id = g_timeout_add (SEARCH_APPLY_TIMEOUT,
                                              (GSourceFunc) mnb_launcher_filter_cb,
                                              self);

  g_free (needle);
}

static void
entry_keynav_cb (MplEntry         *entry,
                 guint             keyval,
                 MnbLauncher      *self)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  NbtkWidget *launcher;
  NbtkWidget *expander;

  if (keyval == CLUTTER_Page_Up)
    {
      NbtkScrollBar *vscroll = NBTK_SCROLL_BAR (
                                  nbtk_scroll_view_get_vscroll_bar (
                                    NBTK_SCROLL_VIEW (priv->scrollview)));
      NbtkAdjustment *adjust = nbtk_scroll_bar_get_adjustment (vscroll);
      gdouble page_size, value;
      g_object_get (adjust,
                    "page-size", &page_size,
                    "value", &value,
                    NULL);
      nbtk_adjustment_set_value (adjust, value - page_size);
    }
  else if (keyval == CLUTTER_Page_Down)
    {
      NbtkScrollBar *vscroll = NBTK_SCROLL_BAR (
                                  nbtk_scroll_view_get_vscroll_bar (
                                    NBTK_SCROLL_VIEW (priv->scrollview)));
      NbtkAdjustment *adjust = nbtk_scroll_bar_get_adjustment (vscroll);
      gdouble page_size, value;
      g_object_get (adjust,
                    "page-size", &page_size,
                    "value", &value,
                    NULL);
      nbtk_adjustment_set_value (adjust, value + page_size);
    }

  if (priv->is_filtering)
    {
      mnb_launcher_keynav_in_grid (self,
                                    MNB_LAUNCHER_GRID (priv->apps_grid),
                                    keyval);
      return;
    }

  /* Favourite apps pane. */
  launcher = mnb_launcher_grid_find_widget_by_pseudo_class (
              MNB_LAUNCHER_GRID (priv->fav_grid),
              "hover");
  if (launcher)
    {
      gboolean keystroke_handled = mnb_launcher_keynav_in_grid (self,
                                    MNB_LAUNCHER_GRID (priv->fav_grid),
                                    keyval);

      /* Move focus to the expanders? */
      if (!keystroke_handled &&
            (keyval == CLUTTER_Down ||
            keyval == CLUTTER_Right))
        {
          NbtkPadding padding;
          nbtk_widget_get_padding (NBTK_WIDGET (priv->apps_grid),
                                    &padding);

          mnb_launcher_grid_keynav_out (MNB_LAUNCHER_GRID (priv->fav_grid));
          expander = mnb_launcher_grid_find_widget_by_point (
                      MNB_LAUNCHER_GRID (priv->apps_grid),
                      padding.left + 1,
                      padding.top + 1);
          if (nbtk_expander_get_expanded (NBTK_EXPANDER (expander)))
            {
              NbtkWidget *inner_grid = NBTK_WIDGET (nbtk_bin_get_child (NBTK_BIN (expander)));
              launcher = mnb_launcher_grid_keynav_first (MNB_LAUNCHER_GRID (inner_grid));
              if (launcher)
                scrollable_ensure_actor_visible (NBTK_SCROLLABLE (priv->scrolled_vbox),
                                                 CLUTTER_ACTOR (launcher));
            }
          else
            mnb_launcher_hover_expander (self,
                                          NBTK_EXPANDER (expander));
        }
      return;
    }

  /* Expander pane - keyboard navigation. */
  expander = mnb_launcher_grid_find_widget_by_pseudo_class (
              MNB_LAUNCHER_GRID (priv->apps_grid),
              "hover");
  if (expander)
    {
      switch (keyval)
        {
          case CLUTTER_Up:
            expander = mnb_launcher_grid_keynav_up (
                        MNB_LAUNCHER_GRID (priv->apps_grid));
            if (expander)
              {
                mnb_launcher_hover_expander (self,
                                              NBTK_EXPANDER (expander));
              }
            break;
          case CLUTTER_Down:
            expander = mnb_launcher_grid_keynav_down (
                        MNB_LAUNCHER_GRID (priv->apps_grid));
            if (expander)
              {
                mnb_launcher_hover_expander (self,
                                              NBTK_EXPANDER (expander));
              }
            break;
          case CLUTTER_Return:
            mnb_launcher_hover_expander (self, NULL);
            nbtk_expander_set_expanded (NBTK_EXPANDER (expander), TRUE);
            break;
        }

      return;
    }


  expander = mnb_launcher_grid_find_widget_by_pseudo_class (
              MNB_LAUNCHER_GRID (priv->apps_grid),
              "active");
  if (expander)
    {
      NbtkWidget *inner_grid = NBTK_WIDGET (nbtk_bin_get_child (NBTK_BIN (expander)));
      gboolean keystroke_handled = mnb_launcher_keynav_in_grid (self,
                                    MNB_LAUNCHER_GRID (inner_grid),
                                    keyval);

      if (!keystroke_handled &&
            (keyval == CLUTTER_Up ||
            keyval == CLUTTER_Left))
        {
          gfloat gap = nbtk_grid_get_row_gap (NBTK_GRID (priv->apps_grid));
          gfloat x = clutter_actor_get_x (CLUTTER_ACTOR (expander));
          gfloat y = clutter_actor_get_y (CLUTTER_ACTOR (expander));

          expander = mnb_launcher_grid_find_widget_by_point (
                      MNB_LAUNCHER_GRID (priv->apps_grid),
                      x + 1,
                      y - gap - 1);
          if (NBTK_IS_EXPANDER (expander))
            mnb_launcher_hover_expander (self,
                                          NBTK_EXPANDER (expander));
          else
            {
              /* Move focus to the fav apps pane. */
              mnb_launcher_grid_keynav_out (MNB_LAUNCHER_GRID (inner_grid));
              launcher = mnb_launcher_grid_keynav_first (MNB_LAUNCHER_GRID (priv->fav_grid));
              if (launcher)
                scrollable_ensure_actor_visible (NBTK_SCROLLABLE (priv->scrolled_vbox),
                                                 CLUTTER_ACTOR (launcher));
            }
        }

      if (!keystroke_handled &&
            (keyval == CLUTTER_Down ||
            keyval == CLUTTER_Right))
        {
          gfloat gap = nbtk_grid_get_row_gap (NBTK_GRID (priv->apps_grid));
          gfloat x = clutter_actor_get_x (CLUTTER_ACTOR (expander));
          gfloat y = clutter_actor_get_y (CLUTTER_ACTOR (expander)) +
                          clutter_actor_get_height (CLUTTER_ACTOR (expander));

          expander = mnb_launcher_grid_find_widget_by_point (
                      MNB_LAUNCHER_GRID (priv->apps_grid),
                      x + 1,
                      y + gap + 1);
          mnb_launcher_hover_expander (self,
                                        NBTK_EXPANDER (expander));
        }

      return;
    }

  /* Nothing is hovered, get first fav app. */
  if (container_has_children (CLUTTER_CONTAINER (priv->fav_grid)))
    {
      launcher = mnb_launcher_grid_keynav_first (MNB_LAUNCHER_GRID (priv->fav_grid));
      if (launcher)
        scrollable_ensure_actor_visible (NBTK_SCROLLABLE (priv->scrolled_vbox),
                                         CLUTTER_ACTOR (launcher));
      return;
    }

  /* Still nothing hovered, get first expander. */
  {
    NbtkPadding padding;
    nbtk_widget_get_padding (NBTK_WIDGET (priv->apps_grid),
                              &padding);
    expander = mnb_launcher_grid_find_widget_by_point (
                MNB_LAUNCHER_GRID (priv->apps_grid),
                padding.left + 1,
                padding.top + 1);
    mnb_launcher_hover_expander (self,
                                  NBTK_EXPANDER (expander));
    return;
  }
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
  MnbLauncherPrivate *priv = GET_PRIVATE (actor);

  clutter_actor_grab_key_focus (priv->filter_entry);
}

static void
_width_notify_cb (MnbLauncher   *self,
                  GParamSpec    *pspec,
                  gpointer       user_data)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);
  GHashTableIter      iter;
  ClutterActor       *expander;

  clutter_actor_set_width (priv->scrollview, SCROLLVIEW_OUTER_WIDTH (self));

  clutter_actor_set_width (priv->fav_grid, SCROLLVIEW_INNER_WIDTH (self));
  clutter_actor_set_width (priv->apps_grid, SCROLLVIEW_INNER_WIDTH (self));

  g_hash_table_iter_init (&iter, priv->expanders);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &expander))
    {
      clutter_actor_set_width (expander, SCROLLVIEW_INNER_WIDTH (self));
    }
}

static void
_height_notify_cb (MnbLauncher   *self,
                   GParamSpec    *pspec,
                   gpointer       user_data)
{
  MnbLauncherPrivate *priv = GET_PRIVATE (self);

  clutter_actor_set_height (priv->scrollview, SCROLLVIEW_OUTER_HEIGHT (self));
}

static GObject *
_constructor (GType                  gtype,
              guint                  n_properties,
              GObjectConstructParam *properties)
{
  MnbLauncher *self = (MnbLauncher *) G_OBJECT_CLASS (mnb_launcher_parent_class)
                                        ->constructor (gtype, n_properties, properties);

  MnbLauncherPrivate *priv = self->priv = REAL_GET_PRIVATE (self);
  ClutterActor    *bar;
  NbtkWidget      *vbox, *label;

  vbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "launcher-vbox");
  nbtk_bin_set_child (NBTK_BIN (self), CLUTTER_ACTOR (vbox));

  /* Filter row. */
  priv->filter_hbox = (ClutterActor *) nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (priv->filter_hbox), "launcher-filter-pane");
  nbtk_table_set_col_spacing (NBTK_TABLE (priv->filter_hbox), 20);
  nbtk_table_add_actor (NBTK_TABLE (vbox), CLUTTER_ACTOR (priv->filter_hbox), 0, 0);

  label = nbtk_label_new (_("Applications"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "launcher-filter-label");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->filter_hbox),
                                        CLUTTER_ACTOR (label),
                                        0, 0,
                                        "y-align", 0.5,
                                        "x-expand", FALSE,
                                        "y-expand", TRUE,
                                        "y-fill", FALSE,
                                        NULL);

  priv->filter_entry = (ClutterActor *) mpl_entry_new (_("Search"));
  clutter_actor_set_name (CLUTTER_ACTOR (priv->filter_entry), "launcher-search-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (priv->filter_entry),
                           FILTER_ENTRY_WIDTH);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->filter_hbox),
                                        CLUTTER_ACTOR (priv->filter_entry),
                                        0, 1,
                                        "y-align", 0.5,
                                        "x-expand", FALSE,
                                        "y-expand", TRUE,
                                        "y-fill", FALSE,
                                        NULL);

  /*
   * Applications
   */
  priv->scrollview = CLUTTER_ACTOR (nbtk_scroll_view_new ());
  nbtk_scroll_view_set_row_size (NBTK_SCROLL_VIEW (priv->scrollview), SCROLLVIEW_ROW_SIZE);
  bar = nbtk_scroll_view_get_vscroll_bar (NBTK_SCROLL_VIEW (priv->scrollview));
#if 0
  /* XXX - the mode was removed from NBTK */
  nbtk_scroll_bar_set_mode (NBTK_SCROLL_BAR (bar), NBTK_SCROLL_BAR_MODE_IDLE);
#endif
  clutter_actor_set_size (priv->scrollview,
                          SCROLLVIEW_OUTER_WIDTH (self), /* account for padding */
                          SCROLLVIEW_OUTER_HEIGHT (self));
  nbtk_table_add_actor_with_properties (NBTK_TABLE (vbox), priv->scrollview, 1, 0,
                                        "x-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-expand", TRUE,
                                        "y-fill", TRUE,
                                        NULL);

  priv->theme = gtk_icon_theme_get_default ();
  g_signal_connect (priv->theme, "changed",
                    G_CALLBACK (mnb_launcher_theme_changed_cb), self);
  priv->manager = mpl_app_bookmark_manager_get_default ();


  priv->scrolled_vbox = CLUTTER_ACTOR (mnb_launcher_grid_new ());
  g_object_set (priv->scrolled_vbox,
                "max-stride", 1,
                NULL);
  clutter_container_add (CLUTTER_CONTAINER (priv->scrollview),
                         priv->scrolled_vbox, NULL);

  mnb_launcher_fill (self);

  /* Track own size. */
  g_signal_connect (self, "notify::width",
                    G_CALLBACK (_width_notify_cb), NULL);
  g_signal_connect (self, "notify::height",
                    G_CALLBACK (_height_notify_cb), NULL);

  /* Hook up search. */
/*
  g_signal_connect_data (entry, "button-clicked",
                         G_CALLBACK (entry_changed_cb), self,
                         (GClosureNotify) mnb_launcher_free_cb, 0);
*/
  g_signal_connect (priv->filter_entry, "button-clicked",
                    G_CALLBACK (mnb_launcher_theme_changed_cb), self);
  g_signal_connect (priv->filter_entry, "text-changed",
                    G_CALLBACK (entry_changed_cb), self);
  g_signal_connect (priv->filter_entry, "keynav-event",
                    G_CALLBACK (entry_keynav_cb), self);

  return (GObject *) self;
}

static void
mnb_launcher_class_init (MnbLauncherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbLauncherPrivate));

  object_class->constructor = _constructor;
  object_class->dispose = _dispose;

  actor_class->key_focus_in = _key_focus_in;

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
{}

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
  GHashTableIter       iter;
  NbtkExpander        *expander;
  NbtkScrollBar       *vscroll;
  NbtkAdjustment      *adjust;

  mpl_entry_set_text (MPL_ENTRY (priv->filter_entry), "");

  /* Close expanders, expand first. */
  g_hash_table_iter_init (&iter, priv->expanders);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &expander))
  {
    if (expander == priv->first_expander &&
        !nbtk_expander_get_expanded (expander))
    {
      nbtk_expander_set_expanded (expander, TRUE);
    }
  }

  /* Reset scroll position. */
  vscroll = NBTK_SCROLL_BAR (nbtk_scroll_view_get_vscroll_bar (
                              NBTK_SCROLL_VIEW (self->priv->scrollview)));
  adjust = nbtk_scroll_bar_get_adjustment (vscroll);
  nbtk_adjustment_set_value (adjust, 0.0);
}


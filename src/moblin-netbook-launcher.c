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

#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>

#include <gtk/gtk.h>
#include <nbtk/nbtk.h>

#include <penge/penge-app-bookmark-manager.h>

#include "moblin-netbook.h"
#include "moblin-netbook-chooser.h"
#include "moblin-netbook-launcher.h"
#include "moblin-netbook-panel.h"
#include "mnb-drop-down.h"
#include "mnb-entry.h"
#include "mnb-launcher-button.h"
#include "mnb-launcher-tree.h"

/*
 * Grid keyboard navigation.
 */

typedef struct
{
  const gchar *pseudo_class;
  NbtkWidget  *widget;
} grid_find_widget_by_pseudo_class_data;

static void
grid_find_widget_by_pseudo_class_cb (ClutterActor                           *actor,
                                     grid_find_widget_by_pseudo_class_data  *data)
{
  const gchar *pseudo_class;

  if (!CLUTTER_ACTOR_IS_VISIBLE (actor))
    return;

  if (!NBTK_IS_STYLABLE (actor))
    return;

  pseudo_class = nbtk_stylable_get_pseudo_class (NBTK_STYLABLE (actor));
  if (0 == g_strcmp0 (data->pseudo_class, pseudo_class))
    data->widget = NBTK_WIDGET (actor);
}

static NbtkWidget *
grid_find_widget_by_pseudo_class (NbtkGrid    *grid,
                                  const gchar *pseudo_class)
{
  grid_find_widget_by_pseudo_class_data data;

  data.pseudo_class = pseudo_class;
  data.widget = NULL;

  clutter_container_foreach (CLUTTER_CONTAINER (grid),
                             (ClutterCallback) grid_find_widget_by_pseudo_class_cb,
                             &data);

  return data.widget;
}

typedef struct {
  ClutterUnit  x;
  ClutterUnit  y;
  NbtkWidget  *widget;
} grid_find_widget_by_point_data_t;

static void
grid_find_widget_by_point_cb (ClutterActor                      *actor,
                              grid_find_widget_by_point_data_t  *data)
{
  ClutterUnit left = clutter_actor_get_xu (actor);
  ClutterUnit top = clutter_actor_get_yu (actor);
  ClutterUnit right = left + clutter_actor_get_widthu (actor);
  ClutterUnit bottom = top + clutter_actor_get_heightu (actor);

  if (CLUTTER_ACTOR_IS_VISIBLE (actor) &&
      left <= data->x &&
      top <= data->y &&
      right >= data->x &&
      bottom >= data->y)
    {
      data->widget = NBTK_WIDGET (actor);
    }
}

static NbtkWidget *
grid_find_widget_by_point (NbtkGrid     *grid,
                           ClutterUnit   x,
                           ClutterUnit   y)
{
  grid_find_widget_by_point_data_t data;

  data.x = x;
  data.y = y;
  data.widget = NULL;

  clutter_container_foreach (CLUTTER_CONTAINER (grid),
                             (ClutterCallback) grid_find_widget_by_point_cb,
                             &data);

  return data.widget;
}

static NbtkWidget *
grid_keynav_up (NbtkGrid *grid)
{
  NbtkWidget  *old, *new;
  ClutterUnit  x, y;

  old = grid_find_widget_by_pseudo_class (grid, "hover");
  if (old == NULL)
    return NULL;

  x = clutter_actor_get_xu (CLUTTER_ACTOR (old)) +
      clutter_actor_get_widthu (CLUTTER_ACTOR (old)) / 2;

  y = clutter_actor_get_yu (CLUTTER_ACTOR (old)) -
      nbtk_grid_get_row_gap (grid) -
      clutter_actor_get_heightu (CLUTTER_ACTOR (old)) / 2;

  new = grid_find_widget_by_point (grid, x, y);
  if (new)
    {
      nbtk_widget_set_style_pseudo_class (old, NULL);
      nbtk_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

static NbtkWidget *
grid_keynav_right (NbtkGrid *grid)
{
  NbtkWidget  *old, *new;
  ClutterUnit  x, y;

  old = grid_find_widget_by_pseudo_class (grid, "hover");
  if (old == NULL)
    return NULL;

  x = clutter_actor_get_xu (CLUTTER_ACTOR (old)) +
      nbtk_grid_get_column_gap (grid) +
      clutter_actor_get_widthu (CLUTTER_ACTOR (old)) * 1.5;

  y = clutter_actor_get_yu (CLUTTER_ACTOR (old)) +
      clutter_actor_get_heightu (CLUTTER_ACTOR (old)) / 2;

  new = grid_find_widget_by_point (grid, x, y);
  if (new)
    {
      nbtk_widget_set_style_pseudo_class (old, NULL);
      nbtk_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

static NbtkWidget *
grid_keynav_down (NbtkGrid *grid)
{
  NbtkWidget  *old, *new;
  ClutterUnit  x, y;

  old = grid_find_widget_by_pseudo_class (grid, "hover");
  if (old == NULL)
    return NULL;

  x = clutter_actor_get_xu (CLUTTER_ACTOR (old)) +
      clutter_actor_get_widthu (CLUTTER_ACTOR (old)) / 2;

  y = clutter_actor_get_yu (CLUTTER_ACTOR (old)) +
      nbtk_grid_get_row_gap (grid) +
      clutter_actor_get_heightu (CLUTTER_ACTOR (old)) * 1.5;

  new = grid_find_widget_by_point (grid, x, y);
  if (new)
    {
      nbtk_widget_set_style_pseudo_class (old, NULL);
      nbtk_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

static NbtkWidget *
grid_keynav_left (NbtkGrid *grid)
{
  NbtkWidget  *old, *new;
  ClutterUnit  x, y;

  old = grid_find_widget_by_pseudo_class (grid, "hover");
  if (old == NULL)
    return NULL;

  x = clutter_actor_get_xu (CLUTTER_ACTOR (old)) -
      nbtk_grid_get_column_gap (grid) -
      clutter_actor_get_widthu (CLUTTER_ACTOR (old)) / 2;

  y = clutter_actor_get_yu (CLUTTER_ACTOR (old)) +
      clutter_actor_get_heightu (CLUTTER_ACTOR (old)) / 2;

  new = grid_find_widget_by_point (grid, x, y);
  if (new)
    {
      nbtk_widget_set_style_pseudo_class (old, NULL);
      nbtk_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

static NbtkWidget *
grid_keynav_wrap_up (NbtkGrid *grid)
{
  NbtkWidget  *old, *new;
  NbtkPadding  padding;
  ClutterUnit  x, y;

  old = grid_find_widget_by_pseudo_class (grid, "hover");
  if (old == NULL)
    return NULL;

  nbtk_widget_get_padding (NBTK_WIDGET (grid), &padding);

  x = clutter_actor_get_widthu (CLUTTER_ACTOR (grid)) -
      padding.right -
      clutter_actor_get_widthu (CLUTTER_ACTOR (old)) / 2;

  y = clutter_actor_get_yu (CLUTTER_ACTOR (old)) -
      nbtk_grid_get_row_gap (grid) -
      clutter_actor_get_heightu (CLUTTER_ACTOR (old)) / 2;

  new = grid_find_widget_by_point (grid, x, y);
  if (new)
    {
      nbtk_widget_set_style_pseudo_class (old, NULL);
      nbtk_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

static NbtkWidget *
grid_keynav_wrap_down (NbtkGrid *grid)
{
  NbtkWidget  *old, *new;
  NbtkPadding  padding;
  ClutterUnit  x, y;

  old = grid_find_widget_by_pseudo_class (grid, "hover");
  if (old == NULL)
    return NULL;

  nbtk_widget_get_padding (NBTK_WIDGET (grid), &padding);

  x = padding.left +
      clutter_actor_get_widthu (CLUTTER_ACTOR (old)) / 2;

  y = clutter_actor_get_yu (CLUTTER_ACTOR (old)) +
      nbtk_grid_get_row_gap (grid) +
      clutter_actor_get_heightu (CLUTTER_ACTOR (old)) * 1.5;

  new = grid_find_widget_by_point (grid, x, y);
  if (new)
    {
      nbtk_widget_set_style_pseudo_class (old, NULL);
      nbtk_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

static NbtkWidget *
grid_keynav_first (NbtkGrid *grid)
{
  NbtkWidget  *widget;
  NbtkPadding  padding;
  ClutterUnit  x, y;

  nbtk_widget_get_padding (NBTK_WIDGET (grid), &padding);

  x = padding.left + 1;
  y = padding.top + 1;

  widget = grid_find_widget_by_point (grid, x, y);
  if (widget)
    {
      nbtk_widget_set_style_pseudo_class (widget, "hover");
      return widget;
    }

  return NULL;
}

static void
grid_keynav_out (NbtkGrid *grid)
{
  NbtkWidget  *widget;

  widget = grid_find_widget_by_pseudo_class (grid, "hover");
  if (widget)
    nbtk_widget_set_style_pseudo_class (widget, NULL);
}

static NbtkWidget *
grid_keynav (NbtkGrid *grid,
             guint     keyval)
{
  NbtkWidget *widget;

  widget = NULL;
  switch (keyval)
    {
      case CLUTTER_Return:
        widget = grid_find_widget_by_pseudo_class (NBTK_GRID (grid), "hover");
        break;
      case CLUTTER_Left:
        widget = grid_keynav_left (NBTK_GRID (grid));
        if (!widget)
          widget = grid_keynav_wrap_up (NBTK_GRID (grid));
        break;
      case CLUTTER_Up:
        widget = grid_keynav_up (NBTK_GRID (grid));
        break;
      case CLUTTER_Right:
        widget = grid_keynav_right (NBTK_GRID (grid));
        if (!widget)
          widget = grid_keynav_wrap_down (NBTK_GRID (grid));
        break;
      case CLUTTER_Down:
        widget = grid_keynav_down (NBTK_GRID (grid));
        break;
    }

  return widget;
}

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

  /* Vertical. */
  if (box->y1 < top)
    nbtk_adjustment_set_value (vadjustment, box->y1);

  if (box->y2 > bottom)
    nbtk_adjustment_set_value (vadjustment, box->y2 - v_page);
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

#define INITIAL_FILL_TIMEOUT_S    4
#define SEARCH_APPLY_TIMEOUT       500
#define LAUNCH_REACTIVE_TIMEOUT_S 2

#define FILTER_ENTRY_WIDTH        600

#define SCROLLBAR_RESERVED_WIDTH  37
#define SCROLLVIEW_RESERVED_WIDTH  3
#define SCROLLVIEW_ROW_SIZE       50
#define EXPANDER_GRID_ROW_GAP      8

#define LAUNCHER_GRID_COLUMN_GAP   32
#define LAUNCHER_GRID_ROW_GAP      12
#define LAUNCHER_WIDTH            210
#define LAUNCHER_HEIGHT            79
#define LAUNCHER_ICON_SIZE         48

/*
 * Helper struct that contains all the info needed to switch between
 * browser- and filter-mode.
 */
typedef struct
{
  MutterPlugin            *self;
  PengeAppBookmarkManager *manager;
  MnbLauncherMonitor      *monitor;
  GHashTable              *expanders;
  GSList                  *launchers;
  guint                    fill_timeout_id;

  /* Static widgets, managed by clutter. */
  ClutterUnit              width;
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
} launcher_data_t;

static void launcher_data_monitor_cb        (MnbLauncherMonitor  *monitor,
                                             launcher_data_t     *launcher_data);

static void launcher_data_set_show_fav_apps (launcher_data_t      *launcher_data,
                                             gboolean              show);

static gboolean
launcher_button_set_reactive_cb (ClutterActor *launcher)
{
  clutter_actor_set_reactive (launcher, TRUE);
  return FALSE;
}

static void
launcher_button_hovered_cb (MnbLauncherButton  *launcher,
                            launcher_data_t    *launcher_data)
{
  NbtkWidget *expander;

  if (launcher_data->is_filtering)
    {
      clutter_container_foreach (CLUTTER_CONTAINER (launcher_data->apps_grid),
                                 (ClutterCallback) nbtk_widget_set_style_pseudo_class,
                                 NULL);
    }
  else
    {
      clutter_container_foreach (CLUTTER_CONTAINER (launcher_data->fav_grid),
                                 (ClutterCallback) nbtk_widget_set_style_pseudo_class,
                                 NULL);

      expander = grid_find_widget_by_pseudo_class (NBTK_GRID (launcher_data->apps_grid),
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
                              MutterPlugin       *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  GAppLaunchContext          *context;
  const gchar     *desktop_file_path;
  const gchar     *exec;
  GDesktopAppInfo *app_info;
  gchar           *last_used;
  GError          *error = NULL;

  /* Disable button for some time to avoid launching multiple times. */
  clutter_actor_set_reactive (CLUTTER_ACTOR (launcher), FALSE);
  g_timeout_add_seconds (LAUNCH_REACTIVE_TIMEOUT_S,
                         (GSourceFunc) launcher_button_set_reactive_cb,
                         launcher);

  context = G_APP_LAUNCH_CONTEXT (gdk_app_launch_context_new ());
  desktop_file_path = mnb_launcher_button_get_desktop_file_path (launcher);
  app_info = g_desktop_app_info_new_from_filename (desktop_file_path);
  g_app_info_launch (G_APP_INFO (app_info), NULL, context, &error);
  if (error)
    g_warning (G_STRLOC "%s", error->message);
  g_clear_error (&error);
  g_object_unref (app_info);
  g_object_unref (context);

  exec = mnb_launcher_button_get_executable (launcher);
  last_used = mnb_launcher_utils_get_last_used (exec);
  mnb_launcher_button_set_comment (launcher, last_used);
  g_free (last_used);

  clutter_actor_hide (priv->launcher);
  nbtk_button_set_checked (NBTK_BUTTON (priv->panel_buttons[5]), FALSE);
  hide_panel (plugin);
}

static void
launcher_button_fav_toggled_cb (MnbLauncherButton  *launcher,
                                launcher_data_t    *launcher_data)
{
  gchar   *uri = NULL;
  GError  *error = NULL;

  if (mnb_launcher_button_get_favorite (launcher))
    {
      NbtkWidget *clone = mnb_launcher_button_create_favorite (launcher);
      clutter_container_add (CLUTTER_CONTAINER (launcher_data->fav_grid),
                             CLUTTER_ACTOR (clone), NULL);
      g_signal_connect (clone, "hovered",
                        G_CALLBACK (launcher_button_hovered_cb),
                        launcher_data);
      g_signal_connect (clone, "activated",
                        G_CALLBACK (launcher_button_activated_cb),
                        launcher_data->self);

      /* Make sure fav apps show up. */
      if (!launcher_data->is_filtering)
        launcher_data_set_show_fav_apps (launcher_data, TRUE);

      /* Update bookmarks. */
      uri = g_strdup_printf ("file://%s",
              mnb_launcher_button_get_desktop_file_path (
                MNB_LAUNCHER_BUTTON (clone)));
      penge_app_bookmark_manager_add_from_uri (launcher_data->manager,
                                               uri,
                                               &error);
    }
  else
    {
      /* Update bookmarks. */
      uri = g_strdup_printf ("file://%s",
              mnb_launcher_button_get_desktop_file_path (
                MNB_LAUNCHER_BUTTON (launcher)));
      penge_app_bookmark_manager_remove_by_uri (launcher_data->manager,
                                                uri,
                                                &error);

      /* Hide fav apps after last one removed. */
      if (!container_has_children (CLUTTER_CONTAINER (launcher_data->fav_grid)))
        {
          launcher_data_set_show_fav_apps (launcher_data, FALSE);
        }
    }

  if (error)
    {
      g_warning ("%s", error->message);
      g_clear_error (&error);
    }

  g_free (uri);
  penge_app_bookmark_manager_save (launcher_data->manager);
}

static NbtkWidget *
launcher_button_create_from_entry (MnbLauncherEntry *entry,
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
                                          LAUNCHER_ICON_SIZE,
                                          GTK_ICON_LOOKUP_GENERIC_FALLBACK);
    }
  if (!info)
    {
      info = gtk_icon_theme_lookup_icon (theme,
                                          "applications-other",
                                          LAUNCHER_ICON_SIZE,
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
      button = mnb_launcher_button_new (icon_file, LAUNCHER_ICON_SIZE,
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

static gboolean
expander_expand_complete_idle_cb (launcher_data_t *launcher_data)
{
  if (nbtk_expander_get_expanded (launcher_data->expand_expander))
    {
      ClutterActor *inner_grid, *launcher;
      inner_grid = nbtk_bin_get_child (NBTK_BIN (launcher_data->expand_expander));
      launcher = (ClutterActor *) grid_keynav_first (NBTK_GRID (inner_grid));

      scrollable_ensure_actor_visible (NBTK_SCROLLABLE (launcher_data->scrolled_vbox),
                                       launcher);

      launcher_data->expand_timeout_id = 0;
      launcher_data->expand_expander = NULL;
    }

  return FALSE;
}

static void
expander_expand_complete_cb (NbtkExpander     *expander,
                             launcher_data_t  *launcher_data)
{
  /* Cancel keyboard navigation to not interfere with the mouse. */
  if (launcher_data->expand_timeout_id)
    {
      g_source_remove (launcher_data->expand_timeout_id);
      launcher_data->expand_timeout_id = 0;
      launcher_data->expand_expander = NULL;
    }

  if (nbtk_expander_get_expanded (expander))
    {
      launcher_data->expand_expander = expander;
      launcher_data->expand_timeout_id = g_idle_add ((GSourceFunc) expander_expand_complete_idle_cb,
                                                     launcher_data);
    }
  else
    {
      ClutterActor *inner_grid;
      inner_grid = nbtk_bin_get_child (NBTK_BIN (expander));
      grid_keynav_out (NBTK_GRID (inner_grid));
    }
}

static void
expander_expanded_notify_cb (NbtkExpander    *expander,
                             GParamSpec      *pspec,
                             launcher_data_t *launcher_data)
{
  NbtkExpander    *e;
  const gchar     *category;
  GHashTableIter   iter;

  /* Cancel keyboard navigation to not interfere with the mouse. */
  if (launcher_data->expand_timeout_id)
    {
      g_source_remove (launcher_data->expand_timeout_id);
      launcher_data->expand_timeout_id = 0;
      launcher_data->expand_expander = NULL;
    }

  /* Close other open expander, so that just the newly opended one is expanded. */
  if (nbtk_expander_get_expanded (expander))
    {
      g_hash_table_iter_init (&iter, launcher_data->expanders);
      while (g_hash_table_iter_next (&iter,
                                     (gpointer *) &category,
                                     (gpointer *) &e))
        {
          if (e != expander)
            {
              ClutterActor *inner_grid = nbtk_bin_get_child (NBTK_BIN (e));
              grid_keynav_out (NBTK_GRID (inner_grid));
              nbtk_expander_set_expanded (e, FALSE);
            }
        }
    }
  else
    {
      ClutterActor *inner_grid = nbtk_bin_get_child (NBTK_BIN (expander));
      grid_keynav_out (NBTK_GRID (inner_grid));
    }
}

static gboolean
expander_expand_cb (launcher_data_t *launcher_data)
{
  launcher_data->expand_timeout_id = 0;
  nbtk_expander_set_expanded (launcher_data->expand_expander, TRUE);
  launcher_data->expand_expander = NULL;

  return FALSE;
}

static void
launcher_data_hover_expander (launcher_data_t *launcher_data,
                              NbtkExpander    *expander)
{
  if (launcher_data->expand_timeout_id)
    {
      g_source_remove (launcher_data->expand_timeout_id);
      launcher_data->expand_timeout_id = 0;
      launcher_data->expand_expander = NULL;
    }

  if (expander)
    {
      nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (expander), "hover");
      launcher_data->expand_expander = expander;
      launcher_data->expand_timeout_id = g_timeout_add (SEARCH_APPLY_TIMEOUT,
                                                        (GSourceFunc) expander_expand_cb,
                                                        launcher_data);
      scrollable_ensure_actor_visible (NBTK_SCROLLABLE (launcher_data->scrolled_vbox),
                                       CLUTTER_ACTOR (expander));
    }
}

static gboolean
launcher_data_keynav_in_grid (launcher_data_t   *launcher_data,
                              NbtkGrid          *grid,
                              guint              keyval)
{
  NbtkWidget *launcher;

  launcher = grid_find_widget_by_pseudo_class (grid, "hover");
  if (launcher)
    {
      /* Do the navigation. */
      launcher = grid_keynav (grid, keyval);
      if (launcher && MNB_IS_LAUNCHER_BUTTON (launcher))
        {
          if (keyval == CLUTTER_Return)
            {
              launcher_button_activated_cb (MNB_LAUNCHER_BUTTON (launcher),
                                            launcher_data->self);
            }
          else
            {
              scrollable_ensure_actor_visible (NBTK_SCROLLABLE (launcher_data->scrolled_vbox),
                                               CLUTTER_ACTOR (launcher));
            }

            return TRUE;
        }
    }
  else
    {
      /* Nothing focused, jump to first actor. */
      launcher = grid_keynav_first (grid);
      if (launcher)
        scrollable_ensure_actor_visible (NBTK_SCROLLABLE (launcher_data->scrolled_vbox),
                                         CLUTTER_ACTOR (launcher));
      return TRUE;
    }

  return FALSE;
}

static void
launcher_data_cancel_search (launcher_data_t *launcher_data)
{
  /* Abort current search if any. */
  if (launcher_data->timeout_id)
    {
      g_source_remove (launcher_data->timeout_id);
      launcher_data->timeout_id = 0;
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

  g_object_unref (launcher_data->fav_label);
  launcher_data->fav_label = NULL;

  g_object_unref (launcher_data->fav_grid);
  launcher_data->fav_grid = NULL;

  g_hash_table_destroy (launcher_data->expanders);
  launcher_data->expanders = NULL;

  g_slist_free (launcher_data->launchers);
  launcher_data->launchers = NULL;
}

static void
launcher_data_set_show_fav_apps (launcher_data_t *launcher_data,
                                 gboolean         show)
{
  if (show && !CLUTTER_ACTOR_IS_VISIBLE (launcher_data->fav_label))
    {
      clutter_actor_show (launcher_data->fav_label);
      clutter_actor_show (launcher_data->fav_grid);
    }
  else if (!show && CLUTTER_ACTOR_IS_VISIBLE (launcher_data->fav_label))
    {
      clutter_actor_hide (launcher_data->fav_label);
      clutter_actor_hide (launcher_data->fav_grid);
    }
}

static gboolean
launcher_data_fill (launcher_data_t *launcher_data)
{
  GList           *fav_apps;
  MnbLauncherTree *tree;
  GSList          *directories;
  GSList const    *directory_iter;
  GtkIconTheme    *theme;

  launcher_data->fill_timeout_id = 0;

  theme = gtk_icon_theme_get_default ();

  if (launcher_data->scrolled_vbox == NULL)
    {
      launcher_data->scrolled_vbox = CLUTTER_ACTOR (nbtk_grid_new ());
      g_object_set (launcher_data->scrolled_vbox,
                    "max-stride", 1,
                    NULL);
      clutter_container_add (CLUTTER_CONTAINER (launcher_data->scrollview),
                             launcher_data->scrolled_vbox, NULL);
    }

  /*
   * Fav apps.
   */

  /* Label */
  launcher_data->fav_label = CLUTTER_ACTOR (nbtk_label_new (_("Favourite Applications")));
  clutter_container_add (CLUTTER_CONTAINER (launcher_data->scrolled_vbox),
                         launcher_data->fav_label, NULL);
  g_object_ref (launcher_data->fav_label);
  clutter_actor_set_name (launcher_data->fav_label, "launcher-group-label");

  /* Grid */
  launcher_data->fav_grid = CLUTTER_ACTOR (nbtk_grid_new ());
  clutter_container_add (CLUTTER_CONTAINER (launcher_data->scrolled_vbox),
                         launcher_data->fav_grid, NULL);
  nbtk_grid_set_row_gap (NBTK_GRID (launcher_data->fav_grid), LAUNCHER_GRID_ROW_GAP);
  nbtk_grid_set_column_gap (NBTK_GRID (launcher_data->fav_grid), LAUNCHER_GRID_COLUMN_GAP);
  clutter_actor_set_width (launcher_data->fav_grid, launcher_data->width);
  clutter_actor_set_name (launcher_data->fav_grid, "launcher-fav-grid");
  g_object_ref (launcher_data->fav_grid);

  fav_apps = penge_app_bookmark_manager_get_bookmarks (launcher_data->manager);
  if (fav_apps)
    {
      GList *fav_apps_iter;

      launcher_data_set_show_fav_apps (launcher_data, TRUE);

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
              button = launcher_button_create_from_entry (entry, NULL, theme);
              mnb_launcher_entry_free (entry);
            }

          if (button)
            {
              mnb_launcher_button_set_favorite (MNB_LAUNCHER_BUTTON (button),
                                                TRUE);
              clutter_container_add (CLUTTER_CONTAINER (launcher_data->fav_grid),
                                     CLUTTER_ACTOR (button), NULL);
              g_signal_connect (button, "hovered",
                                G_CALLBACK (launcher_button_hovered_cb),
                                launcher_data);
              g_signal_connect (button, "activated",
                                G_CALLBACK (launcher_button_activated_cb),
                                launcher_data->self);
              g_signal_connect (button, "fav-toggled",
                                G_CALLBACK (launcher_button_fav_toggled_cb),
                                launcher_data);
            }
        }
      g_list_free (fav_apps);
    }
  else
    {
      launcher_data_set_show_fav_apps (launcher_data, FALSE);
    }

  /*
   * Apps browser.
   */

  /* Grid */
  launcher_data->apps_grid = CLUTTER_ACTOR (nbtk_grid_new ());
  clutter_container_add (CLUTTER_CONTAINER (launcher_data->scrolled_vbox),
                         launcher_data->apps_grid, NULL);
  clutter_actor_set_name (launcher_data->apps_grid, "launcher-apps-grid");
  clutter_actor_set_width (launcher_data->apps_grid, launcher_data->width);
  nbtk_grid_set_row_gap (NBTK_GRID (launcher_data->apps_grid),
                         CLUTTER_UNITS_FROM_INT (EXPANDER_GRID_ROW_GAP));

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
      ClutterActor          *inner_grid;
      NbtkWidget            *button;

      directory = (MnbLauncherDirectory *) directory_iter->data;

      inner_grid = CLUTTER_ACTOR (nbtk_grid_new ());
      nbtk_grid_set_column_gap (NBTK_GRID (inner_grid), LAUNCHER_GRID_COLUMN_GAP);
      nbtk_grid_set_row_gap (NBTK_GRID (inner_grid), LAUNCHER_GRID_ROW_GAP);
      clutter_actor_set_name (inner_grid, "launcher-expander-grid");

      button = NULL;
      for (entry_iter = directory->entries; entry_iter; entry_iter = entry_iter->next)
        {
          button = launcher_button_create_from_entry ((MnbLauncherEntry *) entry_iter->data,
                                                      directory->name,
                                                      theme);
          if (button)
            {
              /* Assuming limited number of fav apps, linear search should do for now. */
              if (launcher_data->fav_grid)
                {
                  clutter_container_foreach (CLUTTER_CONTAINER (launcher_data->fav_grid),
                                             (ClutterCallback) mnb_launcher_button_sync_if_favorite,
                                             button);
                }

              clutter_container_add (CLUTTER_CONTAINER (inner_grid),
                                      CLUTTER_ACTOR (button), NULL);
              g_signal_connect (button, "hovered",
                                G_CALLBACK (launcher_button_hovered_cb),
                                launcher_data);
              g_signal_connect (button, "activated",
                                G_CALLBACK (launcher_button_activated_cb),
                                launcher_data->self);
              g_signal_connect (button, "fav-toggled",
                                G_CALLBACK (launcher_button_fav_toggled_cb),
                                launcher_data);
              launcher_data->launchers = g_slist_prepend (launcher_data->launchers,
                                                          button);
            }
        }

        /* Create expander if at least 1 launcher inside. */
        if (button)
          {
            ClutterActor *expander;

            expander = CLUTTER_ACTOR (nbtk_expander_new ());
            nbtk_expander_set_label (NBTK_EXPANDER (expander),
                                     directory->name);
            clutter_actor_set_width (expander,
                                     launcher_data->width - SCROLLVIEW_RESERVED_WIDTH);
            clutter_container_add (CLUTTER_CONTAINER (launcher_data->apps_grid),
                                   expander, NULL);
            g_hash_table_insert (launcher_data->expanders,
                                 g_strdup (directory->name), expander);
            clutter_container_add (CLUTTER_CONTAINER (expander), inner_grid, NULL);

            /* Open first expander by default. */
            if (directory_iter != directories)
              nbtk_expander_set_expanded (NBTK_EXPANDER (expander), FALSE);

            g_signal_connect (expander, "notify::expanded",
                              G_CALLBACK (expander_expanded_notify_cb),
                              launcher_data);
            g_signal_connect (expander, "expand-complete",
                              G_CALLBACK (expander_expand_complete_cb),
                              launcher_data);
          }
        else
          {
            clutter_actor_destroy (inner_grid);
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

  /* When used as a GSourceFunc this should just run once. */
  return FALSE;
}

/*
 * Ctor.
 */
static launcher_data_t *
launcher_data_new (MutterPlugin *self,
                   ClutterActor *filter_entry,
                   ClutterActor *scrollview,
                   gint          width)
{
  launcher_data_t *launcher_data;

  /* Launcher data instance. */
  launcher_data = g_new0 (launcher_data_t, 1);
  launcher_data->self = self;
  launcher_data->manager = penge_app_bookmark_manager_get_default ();
  penge_app_bookmark_manager_load (launcher_data->manager);

  launcher_data->width = width;
  launcher_data->filter_entry = filter_entry;
  launcher_data->scrollview = scrollview;

  launcher_data->scrolled_vbox = CLUTTER_ACTOR (nbtk_grid_new ());
  g_object_set (launcher_data->scrolled_vbox,
                "max-stride", 1,
                NULL);
  clutter_container_add (CLUTTER_CONTAINER (launcher_data->scrollview),
                         launcher_data->scrolled_vbox, NULL);

  /* Initial fill delayed. */
  launcher_data->fill_timeout_id = g_timeout_add_seconds (INITIAL_FILL_TIMEOUT_S,
                                                          (GSourceFunc) launcher_data_fill,
                                                          launcher_data);
  /* launcher_data_fill (launcher_data); */

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
  launcher_data_reset (launcher_data);
  launcher_data_fill (launcher_data);
}

static gboolean
launcher_data_filter_cb (launcher_data_t *launcher_data)
{
  GSList *iter;

  if (launcher_data->lcase_needle)
    {
      /* Need to switch to filter mode? */
      if (!launcher_data->is_filtering)
        {
          GSList          *iter;
          GHashTableIter   expander_iter;
          ClutterActor    *expander;

          launcher_data->is_filtering = TRUE;
          launcher_data_set_show_fav_apps (launcher_data, FALSE);

          nbtk_grid_set_row_gap (NBTK_GRID (launcher_data->apps_grid),
                                 LAUNCHER_GRID_ROW_GAP);
          nbtk_grid_set_column_gap (NBTK_GRID (launcher_data->apps_grid),
                                    LAUNCHER_GRID_COLUMN_GAP);

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

      /* Perform search. */
      for (iter = launcher_data->launchers; iter; iter = iter->next)
        {
          MnbLauncherButton *button = MNB_LAUNCHER_BUTTON (iter->data);
          mnb_launcher_button_match (button, launcher_data->lcase_needle) ?
            clutter_actor_show (CLUTTER_ACTOR (button)) :
            clutter_actor_hide (CLUTTER_ACTOR (button));
        }

      g_free (launcher_data->lcase_needle);
      launcher_data->lcase_needle = NULL;
    }
  else if (launcher_data->is_filtering)
    {
      /* Did filter, now switch back to normal mode */
      GHashTableIter   expander_iter;
      ClutterActor    *expander;

      launcher_data->is_filtering = FALSE;

      nbtk_grid_set_row_gap (NBTK_GRID (launcher_data->apps_grid),
                             CLUTTER_UNITS_FROM_INT (EXPANDER_GRID_ROW_GAP));
      nbtk_grid_set_column_gap (NBTK_GRID (launcher_data->apps_grid), 0);

      if (container_has_children (CLUTTER_CONTAINER (launcher_data->fav_grid)))
        launcher_data_set_show_fav_apps (launcher_data, TRUE);

      /* Reparent launchers into expanders. */
      for (iter = launcher_data->launchers; iter; iter = iter->next)
        {
          MnbLauncherButton *launcher   = MNB_LAUNCHER_BUTTON (iter->data);
          const gchar       *category   = mnb_launcher_button_get_category (launcher);
          ClutterActor      *e          = g_hash_table_lookup (launcher_data->expanders, category);
          ClutterActor      *inner_grid = nbtk_bin_get_child (NBTK_BIN (e));

          clutter_actor_reparent (CLUTTER_ACTOR (launcher), inner_grid);
        }

      /* Show expanders. */
      g_hash_table_iter_init (&expander_iter, launcher_data->expanders);
      while (g_hash_table_iter_next (&expander_iter, NULL, (gpointer *) &expander))
        {
          clutter_actor_show (expander);
        }
    }

  clutter_actor_queue_relayout (launcher_data->apps_grid);
  return FALSE;
}

static void
entry_changed_cb (MnbEntry         *entry,
                  launcher_data_t  *launcher_data)
{
  gchar *needle;

  launcher_data_cancel_search (launcher_data);

  needle = g_strdup (mnb_entry_get_text (entry));
  needle = g_strstrip (needle);

  if (needle && *needle)
    launcher_data->lcase_needle = g_utf8_strdown (needle, -1);
  launcher_data->timeout_id = g_timeout_add (SEARCH_APPLY_TIMEOUT,
                                              (GSourceFunc) launcher_data_filter_cb,
                                              launcher_data);

  g_free (needle);
}

static void
entry_keynav_cb (MnbEntry         *entry,
                 guint             keyval,
                 launcher_data_t  *launcher_data)
{
  NbtkWidget *launcher;
  NbtkWidget *expander;

  if (launcher_data->is_filtering)
    {
      launcher_data_keynav_in_grid (launcher_data,
                                    NBTK_GRID (launcher_data->apps_grid),
                                    keyval);
      return;
    }

  /* Favourite apps pane. */
  launcher = grid_find_widget_by_pseudo_class (NBTK_GRID (launcher_data->fav_grid),
                                                "hover");
  if (launcher)
    {
      gboolean keystroke_handled = launcher_data_keynav_in_grid (launcher_data,
                                    NBTK_GRID (launcher_data->fav_grid),
                                    keyval);

      /* Move focus to the expanders? */
      if (!keystroke_handled &&
            (keyval == CLUTTER_Down ||
            keyval == CLUTTER_Right))
        {
          NbtkPadding padding;
          nbtk_widget_get_padding (NBTK_WIDGET (launcher_data->apps_grid),
                                    &padding);

          grid_keynav_out (NBTK_GRID (launcher_data->fav_grid));
          expander = grid_find_widget_by_point (NBTK_GRID (launcher_data->apps_grid),
                                                padding.left + 1,
                                                padding.top + 1);
          if (nbtk_expander_get_expanded (NBTK_EXPANDER (expander)))
            {
              NbtkWidget *inner_grid = NBTK_WIDGET (nbtk_bin_get_child (NBTK_BIN (expander)));
              launcher = grid_keynav_first (NBTK_GRID (inner_grid));
              if (launcher)
                scrollable_ensure_actor_visible (NBTK_SCROLLABLE (launcher_data->scrolled_vbox),
                                                 CLUTTER_ACTOR (launcher));
            }
          else
            launcher_data_hover_expander (launcher_data,
                                          NBTK_EXPANDER (expander));
        }
      return;
    }

  /* Expander pane - keyboard navigation. */
  expander = grid_find_widget_by_pseudo_class (NBTK_GRID (launcher_data->apps_grid),
                                               "hover");
  if (expander)
    {
      switch (keyval)
        {
          case CLUTTER_Up:
            expander = grid_keynav_up (NBTK_GRID (launcher_data->apps_grid));
            if (expander)
              {
                launcher_data_hover_expander (launcher_data,
                                              NBTK_EXPANDER (expander));
              }
            break;
          case CLUTTER_Down:
            expander = grid_keynav_down (NBTK_GRID (launcher_data->apps_grid));
              {
                launcher_data_hover_expander (launcher_data,
                                              NBTK_EXPANDER (expander));
              }
            break;
          case CLUTTER_Return:
            launcher_data_hover_expander (launcher_data, NULL);
            nbtk_expander_set_expanded (NBTK_EXPANDER (expander), TRUE);
            break;
        }

      return;
    }


  expander = grid_find_widget_by_pseudo_class (NBTK_GRID (launcher_data->apps_grid),
                                               "active");
  if (expander)
    {
      NbtkWidget *inner_grid = NBTK_WIDGET (nbtk_bin_get_child (NBTK_BIN (expander)));
      gboolean keystroke_handled = launcher_data_keynav_in_grid (launcher_data,
                                    NBTK_GRID (inner_grid),
                                    keyval);

      if (!keystroke_handled &&
            (keyval == CLUTTER_Up ||
            keyval == CLUTTER_Left))
        {
          ClutterUnit gap = nbtk_grid_get_row_gap (NBTK_GRID (launcher_data->apps_grid));
          ClutterUnit x = clutter_actor_get_xu (CLUTTER_ACTOR (expander));
          ClutterUnit y = clutter_actor_get_yu (CLUTTER_ACTOR (expander));

          expander = grid_find_widget_by_point (NBTK_GRID (launcher_data->apps_grid),
                                                x + 1,
                                                y - gap - 1);
          if (NBTK_IS_EXPANDER (expander))
            launcher_data_hover_expander (launcher_data,
                                          NBTK_EXPANDER (expander));
          else
            {
              /* Move focus to the fav apps pane. */
              grid_keynav_out (NBTK_GRID (inner_grid));
              launcher = grid_keynav_first (NBTK_GRID (launcher_data->fav_grid));
              if (launcher)
                scrollable_ensure_actor_visible (NBTK_SCROLLABLE (launcher_data->scrolled_vbox),
                                                 CLUTTER_ACTOR (launcher));
            }
        }

      if (!keystroke_handled &&
            (keyval == CLUTTER_Down ||
            keyval == CLUTTER_Right))
        {
          ClutterUnit gap = nbtk_grid_get_row_gap (NBTK_GRID (launcher_data->apps_grid));
          ClutterUnit x = clutter_actor_get_xu (CLUTTER_ACTOR (expander));
          ClutterUnit y = clutter_actor_get_yu (CLUTTER_ACTOR (expander)) +
                          clutter_actor_get_heightu (CLUTTER_ACTOR (expander));

          expander = grid_find_widget_by_point (NBTK_GRID (launcher_data->apps_grid),
                                                x + 1,
                                                y + gap + 1);
          launcher_data_hover_expander (launcher_data,
                                        NBTK_EXPANDER (expander));
        }

      return;
    }

  /* Nothing is hovered, get first fav app. */
  if (container_has_children (CLUTTER_CONTAINER (launcher_data->fav_grid)))
    {
      launcher = grid_keynav_first (NBTK_GRID (launcher_data->fav_grid));
      if (launcher)
        scrollable_ensure_actor_visible (NBTK_SCROLLABLE (launcher_data->scrolled_vbox),
                                         CLUTTER_ACTOR (launcher));
      return;
    }

  /* Still nothing hovered, get first expander. */
  {
    NbtkPadding padding;
    nbtk_widget_get_padding (NBTK_WIDGET (launcher_data->apps_grid),
                              &padding);
    expander = grid_find_widget_by_point (NBTK_GRID (launcher_data->apps_grid),
                                          padding.left + 1,
                                          padding.top + 1);
    launcher_data_hover_expander (launcher_data,
                                  NBTK_EXPANDER (expander));
    return;
  }
}

static void
dropdown_show_cb (ClutterActor    *actor,
                  launcher_data_t *launcher_data)
{
  /* Cancel timeout and fill if still waiting. */
  if (launcher_data->fill_timeout_id)
    {
      g_source_remove (launcher_data->fill_timeout_id);
      launcher_data_fill (launcher_data);
    }
}

static void
dropdown_show_completed_cb (MnbDropDown     *dropdown,
                            launcher_data_t *launcher_data)
{
  clutter_actor_grab_key_focus (launcher_data->filter_entry);
}

static void
dropdown_hide_cb (MnbDropDown     *dropdown,
                  launcher_data_t *launcher_data)
{
  ClutterActor *stage;

  /* Reset focus. */
  stage = clutter_actor_get_stage (CLUTTER_ACTOR (dropdown));
  clutter_stage_set_key_focus (CLUTTER_STAGE (stage), NULL);

  /* Reset search. */
  mnb_entry_set_text (MNB_ENTRY (launcher_data->filter_entry), "");
}

ClutterActor *
make_launcher (MutterPlugin *plugin,
               gint          width,
               gint          height)
{
  ClutterActor    *scroll, *bar;
  NbtkWidget      *vbox, *hbox, *label, *entry, *drop_down;
  launcher_data_t *launcher_data;

  drop_down = mnb_drop_down_new ();

  vbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "launcher-vbox");
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), CLUTTER_ACTOR (vbox));

  /* Filter row. */
  hbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (hbox), "launcher-filter-pane");
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 20);
  nbtk_table_add_actor (NBTK_TABLE (vbox), CLUTTER_ACTOR (hbox), 0, 0);

  label = nbtk_label_new (_("Applications"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "launcher-filter-label");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                        CLUTTER_ACTOR (label),
                                        0, 0,
                                        "y-align", 0.5,
                                        "x-expand", FALSE,
                                        "y-expand", TRUE,
                                        "y-fill", FALSE,
                                        NULL);

  entry = mnb_entry_new (_("Search"));
  clutter_actor_set_name (CLUTTER_ACTOR (entry), "launcher-search-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (entry),
                           CLUTTER_UNITS_FROM_DEVICE (FILTER_ENTRY_WIDTH));
  nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                        CLUTTER_ACTOR (entry),
                                        0, 1,
                                        "y-align", 0.5,
                                        "x-expand", FALSE,
                                        "y-expand", TRUE,
                                        "y-fill", FALSE,
                                        NULL);

  /*
   * Applications
   */
  scroll = CLUTTER_ACTOR (nbtk_scroll_view_new ());
  nbtk_scroll_view_set_row_size (NBTK_SCROLL_VIEW (scroll), SCROLLVIEW_ROW_SIZE);
  bar = nbtk_scroll_view_get_vscroll_bar (NBTK_SCROLL_VIEW (scroll));
  nbtk_scroll_bar_set_mode (NBTK_SCROLL_BAR (bar), NBTK_SCROLL_BAR_MODE_IDLE);
  clutter_actor_set_size (scroll,
                          width - 10, /* account for padding */
                          height - clutter_actor_get_height (CLUTTER_ACTOR (hbox)));
  nbtk_table_add_actor_with_properties (NBTK_TABLE (vbox), scroll, 1, 0,
                                        "x-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-expand", TRUE,
                                        "y-fill", TRUE,
                                        NULL);

  launcher_data = launcher_data_new (plugin, CLUTTER_ACTOR (entry), scroll,
                                     width - SCROLLBAR_RESERVED_WIDTH);

  /* Hook up search. */
  g_signal_connect_data (entry, "button-clicked",
                         G_CALLBACK (entry_changed_cb), launcher_data,
                         (GClosureNotify) launcher_data_free_cb, 0);
  /* `launcher_data' lifecycle is managed above. */
  g_signal_connect (entry, "text-changed",
                    G_CALLBACK (entry_changed_cb), launcher_data);
  g_signal_connect (entry, "keynav-event",
                    G_CALLBACK (entry_keynav_cb), launcher_data);

  g_signal_connect_after (drop_down, "show",
                          G_CALLBACK (dropdown_show_cb), launcher_data);
  g_signal_connect_after (drop_down, "show-completed",
                          G_CALLBACK (dropdown_show_completed_cb), launcher_data);
  g_signal_connect (drop_down, "hide-completed",
                    G_CALLBACK (dropdown_hide_cb), launcher_data);

  return CLUTTER_ACTOR (drop_down);
}

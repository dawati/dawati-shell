/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *         Chris Lord     <christopher.lord@intel.com>
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

#define GMENU_I_KNOW_THIS_IS_UNSTABLE

#include "moblin-netbook.h"
#include "moblin-netbook-chooser.h"
#include "moblin-netbook-launcher.h"
#include "moblin-netbook-panel.h"
#include "mnb-drop-down.h"
#include "mnb-launcher-button.h"
#include <nbtk/nbtk.h>
#include <gmenu-tree.h>
#include <gtk/gtk.h>
#include <string.h>

#define ICON_SIZE 48
#define PADDING 8
#define BORDER_WIDTH 4
#define N_COLS 4

/* gmenu functions derived from/inspired by gnome-panel, LGPLv2 or later */
static int
compare_applications (GMenuTreeEntry *a,
		      GMenuTreeEntry *b)
{
  return g_utf8_collate (gmenu_tree_entry_get_name (a),
			 gmenu_tree_entry_get_name (b));
}

static GSList *get_all_applications_from_dir (GMenuTreeDirectory *directory,
					      GSList             *list);

static GSList *
get_all_applications_from_alias (GMenuTreeAlias *alias,
				 GSList         *list)
{
  GMenuTreeItem *aliased_item;

  aliased_item = gmenu_tree_alias_get_item (alias);

  switch (gmenu_tree_item_get_type (aliased_item))
    {
    case GMENU_TREE_ITEM_ENTRY:
      list = g_slist_append (list, gmenu_tree_item_ref (aliased_item));
      break;

    case GMENU_TREE_ITEM_DIRECTORY:
      list = get_all_applications_from_dir (GMENU_TREE_DIRECTORY (aliased_item),
					    list);
      break;

    default:
      break;
  }

  gmenu_tree_item_unref (aliased_item);

  return list;
}

static GSList *
get_all_applications_from_dir (GMenuTreeDirectory *directory, GSList *list)
{
  GSList *items;
  GSList *l;

  items = gmenu_tree_directory_get_contents (directory);

  for (l = items; l; l = l->next)
    {
      switch (gmenu_tree_item_get_type (l->data))
	{
	case GMENU_TREE_ITEM_ENTRY:
	  list = g_slist_append (list, gmenu_tree_item_ref (l->data));
	  break;

	case GMENU_TREE_ITEM_DIRECTORY:
	  //list = g_slist_append (list, gmenu_tree_item_ref (l->data));
	  list = get_all_applications_from_dir (l->data, list);
	  break;

	case GMENU_TREE_ITEM_ALIAS:
	  list = get_all_applications_from_alias (l->data, list);
	  break;

	default:
	  break;
	}

    gmenu_tree_item_unref (l->data);
  }

  g_slist_free (items);

  return list;
}

static GSList *
get_all_applications (void)
{
  GMenuTree          *tree;
  GMenuTreeDirectory *root;
  GSList             *retval, *l, *next;
  const gchar        *prev_name;

  tree = gmenu_tree_lookup ("applications.menu", GMENU_TREE_FLAGS_NONE);

  root = gmenu_tree_get_root_directory (tree);

  retval = get_all_applications_from_dir (root, NULL);

  gmenu_tree_item_unref (root);
  gmenu_tree_unref (tree);

  retval = g_slist_sort (retval,
			 (GCompareFunc) compare_applications);

  /* Strip duplicates */
  prev_name = NULL;
  for (l = retval; l; l = next)
    {
      GMenuTreeEntry *entry = l->data;
      const char     *entry_name;

      next = l->next;

      entry_name = gmenu_tree_entry_get_name (entry);
      if (prev_name && entry_name && strcmp (entry_name, prev_name) == 0)
	{
	  gmenu_tree_item_unref (entry);

	  retval = g_slist_delete_link (retval, l);
	}
      else
	{
	  prev_name = entry_name;
	}
  }

  return retval;
}

struct entry_data
{
  gchar        *exec;
  MutterPlugin *plugin;
};

static void
entry_data_free (struct entry_data *data)
{
  g_free (data->exec);
  g_free (data);
}

static void
launcher_activated_cb (MnbLauncherButton  *launcher,
                       ClutterButtonEvent *event,
                       gpointer            data)
{
  struct entry_data           *entry_data = data;
  MutterPlugin                *plugin = entry_data->plugin;
  MoblinNetbookPluginPrivate  *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  const gchar                 *exec = entry_data->exec;
  gboolean                     without_chooser = FALSE;
  gint                         workspace       = -2;

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

  spawn_app (plugin, exec, event->time, without_chooser, workspace);

  clutter_actor_hide (priv->launcher);
  nbtk_button_set_active (NBTK_BUTTON (priv->panel_buttons[5]), FALSE);
  hide_panel (plugin);
}

ClutterActor *
make_launcher (MutterPlugin *plugin,
               gint          width,
               gint          height)
{
  GSList *apps, *a;
  GtkIconTheme  *theme;
  ClutterActor  *table, *view, *scroll;
  gint           row, col;
  struct entry_data *entry_data;
  NbtkWidget    *drop_down;
  NbtkPadding    padding = {CLUTTER_UNITS_FROM_INT (PADDING),
                            CLUTTER_UNITS_FROM_INT (PADDING),
                            CLUTTER_UNITS_FROM_INT (PADDING),
                            CLUTTER_UNITS_FROM_INT (PADDING)};

  table = CLUTTER_ACTOR (nbtk_table_new ());
  nbtk_widget_set_padding (NBTK_WIDGET (table), &padding);
  clutter_actor_set_name (table, "launcher-table");

  nbtk_table_set_col_spacing (NBTK_TABLE (table), PADDING);
  nbtk_table_set_row_spacing (NBTK_TABLE (table), PADDING);

  apps = get_all_applications ();

  theme = gtk_icon_theme_get_default ();

  for (a = apps, row = 0, col = 0; a; a = a->next)
    {
      const gchar       *name, *comment, *icon_file;
      gchar             *exec;

      GMenuTreeEntry *entry = a->data;
      GtkIconInfo *info;

      info = NULL;
      icon_file = NULL;

      exec = g_strdup (gmenu_tree_entry_get_exec (entry));
      name = gmenu_tree_entry_get_icon (entry);
      comment = gmenu_tree_entry_get_comment (entry);
      if (name)
        info = gtk_icon_theme_lookup_icon (theme, name, ICON_SIZE, 0);
      if (info)
        icon_file = gtk_icon_info_get_filename (info);

      if (exec && icon_file)
        {
          NbtkWidget *button;

          button = mnb_launcher_button_new (icon_file, ICON_SIZE,
                                            name, comment, NULL);
          clutter_actor_set_width (CLUTTER_ACTOR (button), 180);
          nbtk_table_add_widget_full (NBTK_TABLE (table), button, row, col,
                                      1, 1, NBTK_KEEP_ASPECT_RATIO, 0, 0);

          entry_data = g_new (struct entry_data, 1);
          entry_data->exec = exec;
          entry_data->plugin = plugin;

          g_signal_connect_data (button, "activated",
                                 G_CALLBACK (launcher_activated_cb), entry_data,
                                 (GClosureNotify) entry_data_free, 0);

          if (++col >= N_COLS)
            {
              col = 0;
              ++row;
            }

          gtk_icon_info_free (info);
        }
      else
        {
          g_free (exec);
        }
    }

  view = nbtk_viewport_new ();
  clutter_container_add (CLUTTER_CONTAINER (view), table, NULL);

  scroll = nbtk_scroll_view_new ();
  clutter_container_add (CLUTTER_CONTAINER (scroll), CLUTTER_ACTOR (view), NULL);

  drop_down = mnb_drop_down_new ();
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), scroll);

  clutter_actor_set_size (scroll, width, height);

  return CLUTTER_ACTOR (drop_down);
}

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
#include "moblin-netbook-launcher.h"
#include <nbtk/nbtk.h>
#include <gmenu-tree.h>
#include <gtk/gtk.h>
#include <string.h>


#define ICON_SIZE 48
#define PADDING 8
#define BORDER_WIDTH 4

extern MutterPlugin mutter_plugin;
static inline MutterPlugin *
mutter_get_plugin ()
{
  return &mutter_plugin;
}

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

static gboolean
entry_input_cb (ClutterActor *icon, ClutterEvent *event, gpointer data)
{
  MutterPlugin       *plugin          = mutter_get_plugin ();
  PluginPrivate      *priv            = plugin->plugin_private;
  const gchar        *exec            = data;
  ClutterButtonEvent *bev             = (ClutterButtonEvent*)event;
  gboolean            without_chooser = FALSE;
  gint                workspace       = -2;

  if (bev->modifier_state & CLUTTER_MOD1_MASK)
    without_chooser = TRUE;
  else
    {
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
    }

  spawn_app (exec, event->any.time, without_chooser, workspace);

  clutter_actor_hide (priv->launcher);

  return TRUE;
}

ClutterActor *
make_launcher (gint width)
{
  GSList *apps, *a;
  GtkIconTheme  *theme;
  ClutterActor  *stage, *launcher;
  gint           row, col, n_cols, pad;
  NbtkWidget    *table, *footer, *up_button;
  NbtkPadding    padding = {CLUTTER_UNITS_FROM_INT (4),
                            CLUTTER_UNITS_FROM_INT (4),
                            CLUTTER_UNITS_FROM_INT (4),
                            CLUTTER_UNITS_FROM_INT (4)};

  n_cols = (width - 2*BORDER_WIDTH) / (ICON_SIZE + PADDING);

  /*
   * Distribute any leftover space into the padding, if possible.
   */
  pad = n_cols*(ICON_SIZE + PADDING) - (width - 2*BORDER_WIDTH);

  if (pad >= n_cols)
    pad /= n_cols;
  else
    pad = 0;

  pad += PADDING;

  table = nbtk_table_new ();
  nbtk_widget_set_padding (table, &padding);
  launcher = CLUTTER_ACTOR (table);
  clutter_actor_set_name (CLUTTER_ACTOR (table), "launcher-table");

  nbtk_table_set_col_spacing (NBTK_TABLE (table), pad);
  nbtk_table_set_row_spacing (NBTK_TABLE (table), pad);

  apps = get_all_applications ();

  theme = gtk_icon_theme_get_default ();

  for (a = apps, row = 0, col = 0; a; a = a->next)
    {
      ClutterActor *icon, *item, *label;
      const gchar  *name;
      gchar        *exec;

      GMenuTreeEntry *entry = a->data;
      GtkIconInfo *info = NULL;

      name = gmenu_tree_entry_get_icon (entry);

      if (!name)
        continue;

      exec = g_strdup (gmenu_tree_entry_get_exec (entry));

      if (!exec)
        continue;

      info = gtk_icon_theme_lookup_icon (theme, name, ICON_SIZE, 0);

      if (!info)
        continue;

      icon = clutter_texture_new_from_file (gtk_icon_info_get_filename (info),
                                            NULL);

      if (icon == NULL)
        continue;

      gtk_icon_info_free (info);

      clutter_actor_set_size (icon, ICON_SIZE, ICON_SIZE);
      g_object_set (G_OBJECT (icon), "sync-size", TRUE, NULL);

      nbtk_table_add_actor (NBTK_TABLE (table), icon, row, col);

      g_signal_connect (icon, "button-press-event",
                        G_CALLBACK (entry_input_cb), exec);

      clutter_actor_set_reactive (icon, TRUE);

      clutter_container_child_set (CLUTTER_CONTAINER (launcher), icon,
				   "keep-aspect-ratio", TRUE, NULL);

      if (++col >= n_cols)
        {
          col = 0;
          ++row;
        }
    }

  table = nbtk_table_new ();

  /* footer with "up" button */
  footer = nbtk_table_new ();
  nbtk_widget_set_padding (footer, &padding);
  nbtk_widget_set_style_class_name (footer, "drop-down-footer");

  up_button = nbtk_button_new ();
  nbtk_widget_set_style_class_name (up_button, "drop-down-up-button");
  nbtk_table_add_actor (NBTK_TABLE (footer), CLUTTER_ACTOR (up_button), 0, 0);
  clutter_actor_set_size (CLUTTER_ACTOR (up_button), 23, 21);
  clutter_container_child_set (CLUTTER_CONTAINER (footer),
                               CLUTTER_ACTOR (up_button),
                               "keep-aspect-ratio", TRUE,
                               "x-align", 1.0,
                               NULL);
  g_signal_connect_swapped (up_button, "clicked", G_CALLBACK (clutter_actor_hide), table);


  /* add all the actors to the group */
  nbtk_table_add_actor (NBTK_TABLE (table), launcher, 0, 0);
  nbtk_table_add_widget (NBTK_TABLE (table), footer, 1, 0);

  clutter_actor_set_width (CLUTTER_ACTOR (table), width);

  return CLUTTER_ACTOR (table);
}

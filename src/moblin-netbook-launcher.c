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
#define PADDING 4
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

  if (bev->modifier_state & CLUTTER_MOD1_MASK)
    without_chooser = TRUE;

  spawn_app (exec, event->any.time, without_chooser);

  clutter_actor_hide (priv->launcher);

  return TRUE;
}

/*
 * FIXME -- this is bit lame; what we really need is for NbtkTable to have
 * row-clicked signal to which we can connect on per-row basis.
 */
static gboolean
table_input_cb (ClutterActor *table, ClutterEvent *event, gpointer data)
{
  /*
   * Processing stops here.
   */
  return TRUE;
}

ClutterActor *
make_launcher (gint width)
{
  GSList *apps, *a;
  GtkIconTheme  *theme;
  ClutterActor  *stage, *launcher;
  NbtkWidget    *table;
  gint           row, col, n_cols, pad;
  ClutterActor  *group, *texture, *footer;
  ClutterColor   bckg_clr = {0xff, 0xff, 0xff, 0xff};
  ClutterActor  *bckg;

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
  launcher = CLUTTER_ACTOR (table);

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

  g_signal_connect (launcher, "button-press-event",
		    G_CALLBACK (table_input_cb), NULL);

  clutter_actor_set_reactive (launcher, TRUE);

  clutter_actor_set_size (launcher,
                          n_cols * (ICON_SIZE + pad),
                          (row + 1) * (ICON_SIZE + pad));

  /*
   * FIXME: This is a dirty hack to add border around the launcher. We really
   *        want to add just padding, but NbtkWidget::allocate() will not run
   *        for subclasses, so the padding property is ignored.
   *        Also, we should match the color to the theme, but attempts to
   *        query the NbtkStylable for background color at this point return
   *        something completely translucent.
   */
  group = clutter_group_new ();
  bckg  = clutter_rectangle_new_with_color (&bckg_clr);

  clutter_actor_set_size (bckg,
                          width,
                          (row + 1) * (ICON_SIZE + pad) + 4*BORDER_WIDTH);

  clutter_actor_set_position (launcher, BORDER_WIDTH, BORDER_WIDTH);

  texture = clutter_texture_new_from_file (PLUGIN_PKGDATADIR "/theme/drop-down/footer.png", NULL);
  footer = nbtk_texture_frame_new (CLUTTER_TEXTURE (texture), 10, 0, 10, 10);
  clutter_actor_set_position (footer, 0, (row + 1) * (ICON_SIZE + pad) + 4*BORDER_WIDTH);
  clutter_actor_set_size (footer, width, 31);


  clutter_container_add (CLUTTER_CONTAINER (group), bckg, launcher, footer, NULL);


  return group;
}

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


#define ICON_SIZE 32

extern MutterPlugin mutter_plugin;
static inline MutterPlugin *
mutter_get_plugin ()
{
  return &mutter_plugin;
}

static int n_rows = 0;
static GList *exec_list = NULL;

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
entry_input_cb (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  clutter_actor_hide (priv->launcher);

  launcher_spawn_app (data);

  return TRUE;
}

/*
 * FIXME -- this is bit lame; what we really need is for NbtkTable to have
 * row-clicked signal to which we can connect on per-row basis.
 */
static gboolean
table_input_cb (ClutterActor *table, ClutterEvent *event, gpointer data)
{
  guint height = clutter_actor_get_height (table);
  guint row_height;
  guint row;
  const gchar *exec;

  row_height = height / n_rows;

  row = ((ClutterButtonEvent*)event)->y / row_height;

  exec = g_list_nth_data (exec_list, row);

  launcher_spawn_app (exec);

  clutter_actor_hide (table);

  return TRUE;
}

ClutterActor *
make_launcher (gint x, gint y)
{
  GSList *apps, *a;
  GtkIconTheme *theme;
  ClutterActor *stage, *launcher;
  NbtkWidget   *table;

  gint i;

  table = nbtk_table_new ();
  launcher = CLUTTER_ACTOR (table);

  apps = get_all_applications ();

  theme = gtk_icon_theme_get_default ();

  for (a = apps, i = 0; a; a = a->next, ++i)
    {
      ClutterActor *icon, *item, *label;
      const gchar  *name;
      gchar        *exec;

      GMenuTreeEntry *entry = a->data;
      GtkIconInfo *info = NULL;

      icon = clutter_texture_new ();
      clutter_actor_set_size (icon, ICON_SIZE, ICON_SIZE);
      g_object_set (G_OBJECT (icon), "sync-size", TRUE, NULL);

      name = gmenu_tree_entry_get_icon (entry);
      exec = g_strdup (gmenu_tree_entry_get_exec (entry));

      if (name)
        info = gtk_icon_theme_lookup_icon (theme, name, ICON_SIZE, 0);

      if (info)
        {
          clutter_texture_set_from_file (CLUTTER_TEXTURE (icon),
                                         gtk_icon_info_get_filename (info),
                                         NULL);

          gtk_icon_info_free (info);
        }

      label = clutter_label_new_with_text ("Sans 12",
					   gmenu_tree_entry_get_name (entry));

      nbtk_table_add_actor (NBTK_TABLE (table), icon, i, 0);
      nbtk_table_add_actor (NBTK_TABLE (table), label, i, 1);

      g_signal_connect (icon, "button-press-event",
			G_CALLBACK (entry_input_cb), exec);
      g_signal_connect (label, "button-press-event",
			G_CALLBACK (entry_input_cb), exec);

      clutter_actor_set_reactive (CLUTTER_ACTOR (icon), TRUE);
      clutter_actor_set_reactive (CLUTTER_ACTOR (label), TRUE);

      clutter_container_child_set (CLUTTER_CONTAINER (launcher), icon,
				   "keep-aspect-ratio", TRUE, NULL);

      exec_list = g_list_prepend (exec_list, exec);
    }

  exec_list = g_list_reverse (exec_list);

  n_rows = i;
  clutter_actor_set_size (launcher, 600, i*ICON_SIZE);

  g_signal_connect (launcher, "button-press-event",
		    G_CALLBACK (table_input_cb), NULL);

  clutter_actor_set_reactive (launcher, TRUE);

  clutter_actor_set_position (launcher, x, y);

  return launcher;
}

/*
 * Panel buttons.
 */
/*
 * Helper method to spawn and application. Will eventually be replaced
 * by libgnome-menu, or something like that.
 */
void
launcher_spawn_app (const gchar *path)
{
  MutterPlugin      *plugin    = mutter_get_plugin ();
  PluginPrivate     *priv      = plugin->plugin_private;
  MetaScreen        *screen    = mutter_plugin_get_screen (plugin);
  MetaDisplay       *display   = meta_screen_get_display (screen);
  guint32            timestamp = meta_display_get_current_time (display);

  Display           *xdpy = mutter_plugin_get_xdisplay (plugin);
  SnLauncherContext *context = NULL;
  const gchar       *id;
  gchar             *argv[2] = {NULL, NULL};

  argv[0] = g_strdup (path);

  if (!path)
    return;

  context = sn_launcher_context_new (priv->sn_display, DefaultScreen (xdpy));

  /* FIXME */
  sn_launcher_context_set_name (context, path);
  sn_launcher_context_set_description (context, path);
  sn_launcher_context_set_binary_name (context, path);

  sn_launcher_context_initiate (context,
                                "mutter-netbook-shell",
                                path, /* bin_name */
				timestamp);

  id = sn_launcher_context_get_startup_id (context);

  if (!g_spawn_async (NULL,
                      &argv[0],
                      NULL,
                      G_SPAWN_SEARCH_PATH,
                      (GSpawnChildSetupFunc)
                      sn_launcher_context_setup_child_process,
                      (gpointer)context,
                      NULL,
                      NULL))
    {
      g_warning ("Failed to launch [%s]", path);
    }

  sn_launcher_context_unref (context);
}


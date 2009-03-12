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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gmenu-tree.h>
#include <nbtk/nbtk.h>

#include <penge/penge-utils.h>

#include "moblin-netbook.h"
#include "moblin-netbook-chooser.h"
#include "moblin-netbook-launcher.h"
#include "moblin-netbook-panel.h"
#include "mnb-drop-down.h"
#include "mnb-entry.h"
#include "mnb-launcher-button.h"

#define WIDGET_SPACING 5
#define ICON_SIZE 48
#define PADDING 8
#define N_COLS 4
#define LAUNCHER_WIDTH 235

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

typedef struct
{
  gchar        *exec;
  MutterPlugin *plugin;
} entry_data_t;

static void
entry_data_free (entry_data_t *data)
{
  g_free (data->exec);
  g_free (data);
}

static void
launcher_activated_cb (MnbLauncherButton  *launcher,
                       ClutterButtonEvent *event,
                       entry_data_t       *entry_data)
{
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

  moblin_netbook_spawn (plugin, exec, event->time, without_chooser, workspace);

  clutter_actor_hide (priv->launcher);
  nbtk_button_set_active (NBTK_BUTTON (priv->panel_buttons[5]), FALSE);
  hide_panel (plugin);
}

static gchar *
get_generic_name (GMenuTreeEntry *entry)
{
  const gchar *desktop_file;
  GKeyFile    *key_file = NULL;
  GError      *error = NULL;
  gchar       *generic_name = NULL;

  g_return_val_if_fail (entry, NULL);

  desktop_file = gmenu_tree_entry_get_desktop_file_path (entry);
  if (!desktop_file)
    goto bail;

  key_file = g_key_file_new ();
  g_key_file_load_from_file (key_file, desktop_file, G_KEY_FILE_NONE, &error);
  if (error)
    {
      g_warning ("%s", error->message);
      goto bail;
    }

  generic_name = g_key_file_get_string (key_file,
                                        G_KEY_FILE_DESKTOP_GROUP,
                                        G_KEY_FILE_DESKTOP_KEY_GENERIC_NAME,
                                        &error);
  if (error)
    {
      /* g_message ("%s", error->message); */
      goto bail;
    }

bail:
  if (key_file) g_key_file_free (key_file);
  if (error) g_error_free (error);
  /* Fall back to "Name" if no "GenericName" available. */
  if (!generic_name)
    generic_name = g_strdup (gmenu_tree_entry_get_name (entry));

  return generic_name;
}

/*
 * Get executable from menu entry and check it's available in the path.
 * Returns: absolute path if found, otherwise NULL.
 */
static gchar *
get_exec (GMenuTreeEntry *entry)
{
  const gchar  *exec;
  gint          argc;
  gchar       **argv;
  GError       *error;
  
  exec = gmenu_tree_entry_get_exec (entry);
  if (!exec)
    return NULL;
    
  error = NULL;
  if (g_shell_parse_argv (exec, &argc, &argv, &error))
    {
      char *binary = g_find_program_in_path (argv[0]);
      g_strfreev (argv);
      return binary;      
    }
    
  g_warning ("%s", error->message);
  g_error_free (error);
    
  return NULL;
}

static NbtkGrid *
make_table (MutterPlugin  *self)
{
  ClutterActor  *grid;
  GSList *apps, *a;
  GtkIconTheme  *theme;
  entry_data_t  *entry_data;

  NbtkPadding    padding = {CLUTTER_UNITS_FROM_INT (PADDING),
                            CLUTTER_UNITS_FROM_INT (PADDING),
                            CLUTTER_UNITS_FROM_INT (PADDING),
                            CLUTTER_UNITS_FROM_INT (PADDING)};

  grid = nbtk_grid_new ();
  clutter_actor_set_width (grid, 4 * LAUNCHER_WIDTH + 5 * PADDING);
  nbtk_widget_set_padding (NBTK_WIDGET (grid), &padding);
  clutter_actor_set_name (grid, "app-launcher-table");

  nbtk_grid_set_row_gap (NBTK_GRID (grid), CLUTTER_UNITS_FROM_INT (PADDING));
  nbtk_grid_set_column_gap (NBTK_GRID (grid), CLUTTER_UNITS_FROM_INT (PADDING));

  apps = get_all_applications ();
  theme = gtk_icon_theme_get_default ();

  for (a = apps; a; a = a->next)
    {
      const gchar   *name, *description, *icon_file;
      gchar         *generic_name, *exec, *last_used;
      struct stat    exec_stat;

      GMenuTreeEntry *entry = a->data;
      GtkIconInfo *info;

      info = NULL;
      icon_file = NULL;

      generic_name = get_generic_name (entry);
      exec = get_exec (entry);
      name = gmenu_tree_entry_get_icon (entry);
      description = gmenu_tree_entry_get_comment (entry);

      if (name)
        info = gtk_icon_theme_lookup_icon (theme, name, ICON_SIZE, 0);
      else
        info = gtk_icon_theme_lookup_icon (theme, "gtk-file", ICON_SIZE, 0);
      if (info)
        icon_file = gtk_icon_info_get_filename (info);

      if (generic_name && exec && icon_file)
        {
          NbtkWidget *button;

          /* FIXME robsta: read "last launched" from persist cache once we have that.
           * For now approximate. */
          last_used = NULL;
          if (0 == stat (exec, &exec_stat) &&
              exec_stat.st_atime != exec_stat.st_mtime)
            {
              GTimeVal atime = { 0 ,0 };
              atime.tv_sec = exec_stat.st_atime;
              last_used = penge_utils_format_time (&atime);
            }

          button = mnb_launcher_button_new (icon_file, ICON_SIZE,
                                            generic_name, description, last_used);
          g_free (last_used);
          clutter_actor_set_width (CLUTTER_ACTOR (button), LAUNCHER_WIDTH);
          clutter_container_add (CLUTTER_CONTAINER (grid),
                                 CLUTTER_ACTOR (button), NULL);

          entry_data = g_new (entry_data_t, 1);
          entry_data->exec = exec;
          entry_data->plugin = self;
          g_signal_connect_data (button, "activated",
                                 G_CALLBACK (launcher_activated_cb), entry_data,
                                 (GClosureNotify) entry_data_free, 0);
        }
      else
        {
          g_free (exec);
        }

      if (info) gtk_icon_info_free (info);
      g_free (generic_name);
    }

    return NBTK_GRID (grid);
}

static void
filter_cb (ClutterActor *actor,
           const gchar *filter_key)
{
  MnbLauncherButton *button;
  const char        *title;
  const char        *description;
  const char        *comment;

  button = MNB_LAUNCHER_BUTTON (actor);
  g_return_if_fail (button);

  /* Show all? */    
  if (!filter_key || strlen (filter_key) == 0)
    {
      clutter_actor_show (CLUTTER_ACTOR (button));
      return;
    }

  title = mnb_launcher_button_get_title (button);
  if (title)
    {
      gchar *title_key = g_utf8_strdown (title, -1);
      gboolean is_matching = (gboolean) strstr (title_key, filter_key);
      g_free (title_key);
      if (is_matching)
        {
          clutter_actor_show (CLUTTER_ACTOR (button));
          return;
        }
    }

  description = mnb_launcher_button_get_description (button);
  if (description)
    {
      gchar *description_key = g_utf8_strdown (description, -1);
      gboolean is_matching = (gboolean) strstr (description_key, filter_key);
      g_free (description_key);
      if (is_matching)
        {
          clutter_actor_show (CLUTTER_ACTOR (button));
          return;        
        }
    }

  comment = mnb_launcher_button_get_comment (button);
  if (comment)
    {
      gchar *comment_key = g_utf8_strdown (comment, -1);
      gboolean is_matching = (gboolean) strstr (comment_key, filter_key);
      g_free (comment_key);
      if (is_matching)
        {
          clutter_actor_show (CLUTTER_ACTOR (button));
          return;        
        }
    }

  /* No match. */
  clutter_actor_hide (CLUTTER_ACTOR (button));
}

typedef struct
{
  MutterPlugin  *plugin;
  NbtkGrid      *grid;
} search_data_t;

static void
search_activated_cb (MnbEntry       *entry,
                     search_data_t  *data)
{
  ClutterActor  *launcher_table;
  GList         *children, *child;
  gchar         *filter, *key;

  filter = NULL;
  g_object_get (entry, "text", &filter, NULL);
  key = g_utf8_strdown (filter, -1);
  g_free (filter), filter = NULL;

  clutter_container_foreach (CLUTTER_CONTAINER (data->grid), 
                             (ClutterCallback) filter_cb,
                             key);
}

ClutterActor *
make_launcher (MutterPlugin *plugin,
               gint          width,
               gint          height)
{
  ClutterActor  *viewport, *scroll;
  NbtkWidget    *vbox, *hbox, *label, *entry, *drop_down;
  search_data_t *search_data;
  NbtkPadding    padding = {CLUTTER_UNITS_FROM_INT (PADDING),
                            0, 0, 0};
  NbtkPadding    hbox_padding = { CLUTTER_UNITS_FROM_INT (PADDING),
                                  CLUTTER_UNITS_FROM_INT (PADDING),
                                  CLUTTER_UNITS_FROM_INT (PADDING),
                                  CLUTTER_UNITS_FROM_INT (PADDING)};


  drop_down = mnb_drop_down_new ();

  vbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "app-launcher-vbox");
  nbtk_table_set_row_spacing (NBTK_TABLE (vbox), WIDGET_SPACING);
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), CLUTTER_ACTOR (vbox));

  /* 1st row: Filter. */
  hbox = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), WIDGET_SPACING);
  nbtk_table_add_widget (NBTK_TABLE (vbox), hbox, 0, 0);
  nbtk_widget_set_padding (NBTK_WIDGET (hbox), &hbox_padding);

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

  viewport = nbtk_viewport_new ();
  /* Add launcher table. */
  search_data = g_new0 (search_data_t, 1);
  search_data->plugin = plugin;
  search_data->grid = make_table (plugin);
  clutter_container_add (CLUTTER_CONTAINER (viewport),
                         CLUTTER_ACTOR (search_data->grid), NULL);


  scroll = nbtk_scroll_view_new ();
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
                         G_CALLBACK (search_activated_cb), search_data,
                         (GClosureNotify) g_free, 0);
  /* `search_data' lifecycle is managed above. */
  g_signal_connect (entry, "text-changed",
                    G_CALLBACK (search_activated_cb), search_data);

  return CLUTTER_ACTOR (drop_down);
}

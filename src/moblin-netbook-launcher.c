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

typedef struct {
  GSList *head;
} entry_list_head_t;

/* gmenu functions derived from/inspired by gnome-panel, LGPLv2 or later */

static void get_all_applications_from_dir (GMenuTreeDirectory *directory,
                                           const gchar        *category,
					                                 GHashTable         *apps_hash);

/*
 * Lookup the list-head for a category of entries.
 *
 * Empty list is instantiated if it doesn't already exist.
 */
static entry_list_head_t *
lookup_entry_list (GHashTable   *apps_hash,
                   const gchar  *category)
{
  entry_list_head_t *category_list;

  category_list = g_hash_table_lookup (apps_hash, category);
  if (!category_list)
    {
      category_list = g_new0 (entry_list_head_t, 1);
      g_hash_table_insert (apps_hash, g_strdup (category), category_list);
    }

  return category_list;
}

static void
get_all_applications_from_alias (GMenuTreeAlias *alias,
                                 const gchar    *category,
                                 GHashTable     *apps_hash)
{
  GMenuTreeItem     *aliased_item;
  entry_list_head_t *category_list;

  aliased_item = gmenu_tree_alias_get_item (alias);

  switch (gmenu_tree_item_get_type (aliased_item))
    {
    case GMENU_TREE_ITEM_ENTRY:
      category_list = lookup_entry_list (apps_hash, category);
      /* Consume aliased_item ref. */
      category_list->head = g_slist_prepend (category_list->head, aliased_item);
      break;

    case GMENU_TREE_ITEM_DIRECTORY:
      get_all_applications_from_dir (
            GMENU_TREE_DIRECTORY (aliased_item),
            gmenu_tree_directory_get_name (GMENU_TREE_DIRECTORY (aliased_item)),
            apps_hash);
      gmenu_tree_item_unref (aliased_item);
      break;

    default:
      break;
  }
}

static void
get_all_applications_from_dir (GMenuTreeDirectory *directory,
                               const gchar        *category,
                               GHashTable         *apps_hash)
{
  entry_list_head_t *category_list;
  GSList            *list, *iter;

  category_list = NULL;
  if (category)
    category_list = lookup_entry_list (apps_hash, category);

  list = gmenu_tree_directory_get_contents (directory);

  for (iter = list; iter; iter = iter->next)
    {
      switch (gmenu_tree_item_get_type (iter->data))
      	{
        	case GMENU_TREE_ITEM_ENTRY:
            /* Consume entry ref. */
            if (category_list)
              category_list->head = g_slist_prepend (category_list->head,
                                                     iter->data);
        	  break;

        	case GMENU_TREE_ITEM_DIRECTORY:
        	  get_all_applications_from_dir (
        	      GMENU_TREE_DIRECTORY (iter->data),
        	      gmenu_tree_directory_get_name (GMENU_TREE_DIRECTORY (iter->data)),
        	      apps_hash);
            gmenu_tree_item_unref (iter->data);
        	  break;

        	case GMENU_TREE_ITEM_ALIAS:
        	  get_all_applications_from_alias (
        	      GMENU_TREE_ALIAS (iter->data),
        	      category,
        	      apps_hash);
            gmenu_tree_item_unref (iter->data);
        	  break;

        	default:
        	  break;
      	}
  }

  g_slist_free (list);
}

static void
entry_list_free (entry_list_head_t *list)
{
  GSList *iter;

  iter = list->head;
  while (iter)
    {
      gmenu_tree_item_unref (GMENU_TREE_ITEM (iter->data));
      iter = g_slist_delete_link (iter, iter);
    }

  g_free (list);
}

static GHashTable *
get_all_applications (void)
{
  GMenuTree          *tree;
  GMenuTreeDirectory *root;
  GHashTable         *apps_hash;
#if 0
  GSList             *l, *next;
  const gchar        *prev_name;
#endif
  /* FIXME: also merge "settings.menu" or whatever its called. */
  tree = gmenu_tree_lookup ("applications.menu", GMENU_TREE_FLAGS_NONE);

  root = gmenu_tree_get_root_directory (tree);
  apps_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                     g_free, (GDestroyNotify) entry_list_free);

  get_all_applications_from_dir (root, NULL, apps_hash);

  gmenu_tree_item_unref (root);
  gmenu_tree_unref (tree);

  /* TODO: strip duplicates: really needed? */
#if 0
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
#endif
  return apps_hash;
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
  nbtk_button_set_checked (NBTK_BUTTON (priv->panel_buttons[5]), FALSE);
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
  ClutterActor      *grid;
  GHashTable        *apps;
  GHashTableIter     iter;
  entry_list_head_t *list;
  const gchar       *category;
  GtkIconTheme      *theme;

  grid = CLUTTER_ACTOR (nbtk_grid_new ());
  clutter_actor_set_width (grid, 4 * LAUNCHER_WIDTH + 5 * PADDING);
  clutter_actor_set_name (grid, "app-launcher-table");

  nbtk_grid_set_row_gap (NBTK_GRID (grid), CLUTTER_UNITS_FROM_INT (PADDING));
  nbtk_grid_set_column_gap (NBTK_GRID (grid), CLUTTER_UNITS_FROM_INT (PADDING));

  apps = get_all_applications ();
  theme = gtk_icon_theme_get_default ();

  g_hash_table_iter_init (&iter, apps);
  while (g_hash_table_iter_next (&iter, (gpointer *) &category, (gpointer *) &list))
    {
      GSList        *category_iter;
      ClutterActor  *expander, *inner_grid;

      expander = CLUTTER_ACTOR (nbtk_expander_new (category));
      clutter_actor_set_width (expander, 4 * LAUNCHER_WIDTH + 5 * PADDING);
      clutter_container_add (CLUTTER_CONTAINER (grid), expander, NULL);

      inner_grid = CLUTTER_ACTOR (nbtk_grid_new ());
      clutter_container_add (CLUTTER_CONTAINER (expander), inner_grid, NULL);

      for (category_iter = list->head; category_iter; category_iter = category_iter->next)
        {
          const gchar   *name, *description, *icon_file;
          gchar         *generic_name, *exec, *last_used;
          struct stat    exec_stat;

          GMenuTreeEntry *entry = category_iter->data;
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
              NbtkWidget    *button;
              entry_data_t  *entry_data;

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
                                                generic_name, category,
                                                description, last_used);
              g_free (last_used);
              clutter_actor_set_width (CLUTTER_ACTOR (button), LAUNCHER_WIDTH);
              clutter_container_add (CLUTTER_CONTAINER (inner_grid),
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
    }

    return NBTK_GRID (grid);
}

static void
filter_cb (ClutterActor *actor,
           const gchar  *filter_key)
{
  MnbLauncherButton *button;
  const char        *title;
  const char        *description;
  const char        *comment;

  /* The grid contains both, buttons and expanders for the respective search
   * mode. Skip over expanders while searching, they are invisible anyway. */
  if (!MNB_IS_LAUNCHER_BUTTON (actor))
    return;

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
  GHashTable    *expanders;
  gboolean       is_filtering;
} search_data_t;

static search_data_t *
search_data_new (MutterPlugin *plugin,
                 NbtkGrid     *grid)
{
  search_data_t *search_data;

  search_data = g_new0 (search_data_t, 1);
  search_data->plugin = plugin;
  search_data->grid = grid;
  search_data->expanders = g_hash_table_new (g_str_hash, g_str_equal);

  return search_data;
}

static void
search_data_fill_cb (ClutterActor   *expander,
                     search_data_t  *search_data)
{
  g_hash_table_insert (search_data->expanders,
                       (gpointer) nbtk_expander_get_label (NBTK_EXPANDER (expander)),
                       expander);
}

static void
search_data_free_cb (search_data_t *search_data)
{
  g_hash_table_destroy (search_data->expanders);
  g_free (search_data);
}

typedef struct
{
  ClutterContainer *from;
  ClutterContainer *to;
} reparent_t;

static void
reparent_cb (ClutterActor *button,
             reparent_t   *containers)
{
  g_object_ref (button);
  clutter_container_remove (containers->from, button, NULL);
  clutter_container_add (containers->to, button, NULL);
  g_object_unref (button);
}

static void
search_activated_cb (MnbEntry       *entry,
                     search_data_t  *search_data)
{
  gchar *filter, *key;

  filter = NULL;
  g_object_get (entry, "text", &filter, NULL);
  key = g_utf8_strdown (filter, -1);
  g_free (filter), filter = NULL;

  if (key && strlen (key) > 0)
    {
      /* Do filter.
       * Need to switch to filter mode? */
      if (!search_data->is_filtering)
        {
          const gchar     *category;
          ClutterActor    *expander;
          GHashTableIter   iter;

          search_data->is_filtering = TRUE;

          /* Go thru expanders, hide them, and reparent the buttons
           * into the main grid. */
          g_hash_table_iter_init (&iter, search_data->expanders);
          while (g_hash_table_iter_next (&iter, (gpointer *) &category, (gpointer *) &expander))
            {
              ClutterActor *child;
              reparent_t    containers;

              clutter_actor_hide (expander);

              child = nbtk_expander_get_child (NBTK_EXPANDER (expander));
              containers.from = CLUTTER_CONTAINER (child);
              containers.to = CLUTTER_CONTAINER (search_data->grid);
              clutter_container_foreach (CLUTTER_CONTAINER (child),
                                         (ClutterCallback) reparent_cb,
                                         &containers);
            }
        }

      /* Update search result. */
      clutter_container_foreach (CLUTTER_CONTAINER (search_data->grid),
                                 (ClutterCallback) filter_cb,
                                 key);
    }
  else if (search_data->is_filtering &&
           (!key || strlen (key) == 0))
    {
      /* Did filter, now switch back to normal mode */
      GList *children;

      search_data->is_filtering = FALSE;

      children = clutter_container_get_children (CLUTTER_CONTAINER (search_data->grid));
      while (children)
        {
          ClutterActor *child = CLUTTER_ACTOR (children->data);

          if (NBTK_IS_EXPANDER (child))
            {
              /* Expanders are visible again. */
              clutter_actor_show (child);
            }
          else
            {
              /* Buttons go back into category expanders. */
              const gchar *category = mnb_launcher_button_get_category (
                                          MNB_LAUNCHER_BUTTON (child));
              ClutterActor  *expander = g_hash_table_lookup (
                                          search_data->expanders,
                                          category);
              ClutterActor *inner_grid = nbtk_expander_get_child (
                                          NBTK_EXPANDER (expander));

              g_object_ref (child);
              clutter_container_remove (CLUTTER_CONTAINER (search_data->grid),
                                        child, NULL);
              clutter_container_add (CLUTTER_CONTAINER (inner_grid),
                                     child, NULL);
              g_object_unref (child);
            }

          children = g_list_delete_link (children, children);
        }
    }

  g_free (key);
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
  ClutterActor  *viewport, *scroll;
  NbtkWidget    *vbox, *hbox, *label, *entry, *drop_down;
  search_data_t *search_data;

  drop_down = mnb_drop_down_new ();

  vbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "app-launcher-vbox");
  nbtk_table_set_row_spacing (NBTK_TABLE (vbox), WIDGET_SPACING);
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), CLUTTER_ACTOR (vbox));

  /* 1st row: Filter. */
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

  viewport = CLUTTER_ACTOR (nbtk_viewport_new ());
  /* Add launcher table. */
  search_data = search_data_new (plugin,
                                 make_table (plugin));
  clutter_container_add (CLUTTER_CONTAINER (viewport),
                         CLUTTER_ACTOR (search_data->grid), NULL);
  /* Keep all launcher buttons to switch between browse- and filter-mode. */
  clutter_container_foreach (CLUTTER_CONTAINER (search_data->grid),
                             (ClutterCallback) search_data_fill_cb,
                             search_data);


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
                         G_CALLBACK (search_activated_cb), search_data,
                         (GClosureNotify) search_data_free_cb, 0);
  /* `search_data' lifecycle is managed above. */
  g_signal_connect (entry, "text-changed",
                    G_CALLBACK (search_activated_cb), search_data);

  return CLUTTER_ACTOR (drop_down);
}

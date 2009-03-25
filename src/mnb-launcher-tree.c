/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
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

#include <sys/stat.h>

#define GMENU_I_KNOW_THIS_IS_UNSTABLE
#include <gmenu-tree.h>

#include <penge/penge-utils.h>

#include "mnb-launcher-tree.h"
#include "moblin-netbook.h"

/*
 * MnbLauncherMonitor.
 */

struct MnbLauncherMonitor_ {
  GMenuTree                   *applications;
  GMenuTree                   *settings;
  MnbLauncherMonitorFunction   monitor_function;
  gpointer                     user_data;
};

static gboolean
menu_changed_idle_cb (MnbLauncherMonitor *self)
{
  self->monitor_function (self, self->user_data);

  return FALSE;
}

static void
applications_menu_changed_cb (GMenuTree           *tree,
                              MnbLauncherMonitor  *self)
{
  g_return_if_fail (self);

  /* Filter multiple notifications. */
  g_idle_remove_by_data (self);
  g_idle_add_full (G_PRIORITY_LOW,
                   (GSourceFunc) menu_changed_idle_cb,
                   self, NULL);
}

static void
settings_menu_changed_cb (GMenuTree           *tree,
                          MnbLauncherMonitor  *self)
{
  g_return_if_fail (self);

  /* Filter multiple notifications. */
  g_idle_remove_by_data (self);
  g_idle_add_full (G_PRIORITY_LOW,
                   (GSourceFunc) menu_changed_idle_cb,
                   self, NULL);
}

void
mnb_launcher_monitor_free (MnbLauncherMonitor *self)
{
  g_return_if_fail (self);

  gmenu_tree_remove_monitor (self->applications,
                             (GMenuTreeChangedFunc) applications_menu_changed_cb,
                             self);
  gmenu_tree_unref (self->applications);

  gmenu_tree_remove_monitor (self->settings,
                             (GMenuTreeChangedFunc) settings_menu_changed_cb,
                             self);
  gmenu_tree_unref (self->settings);

  g_free (self);
}

/*
 * MnbLauncherEntry.
 */

struct MnbLauncherEntry_ {
  GMenuTreeEntry  *entry;
  gchar           *generic_name;
};

static MnbLauncherEntry *
mnb_launcher_entry_new (GMenuTreeEntry *entry)
{
  MnbLauncherEntry *self;

  g_return_val_if_fail (entry, NULL);

  self = g_new0 (MnbLauncherEntry, 1);
  self->entry = gmenu_tree_item_ref (entry);

  return self;
}

static void
mnb_launcher_entry_free (MnbLauncherEntry *self)
{
  g_return_if_fail (self);

  gmenu_tree_item_unref (self->entry);
  g_free (self->generic_name);
  g_free (self);
}

static gint
mnb_launcher_entry_compare (MnbLauncherEntry *self,
                            MnbLauncherEntry *b)
{
  return g_utf8_collate (mnb_launcher_entry_get_name (self),
                         mnb_launcher_entry_get_name (b));
}

/*
 * MnbLauncherDirectory.
 */

static MnbLauncherDirectory *
mnb_launcher_directory_new (GMenuTreeDirectory *branch)
{
  MnbLauncherDirectory *self;

  g_return_val_if_fail (branch, NULL);

  self = g_new0 (MnbLauncherDirectory, 1);
  self->name = g_strdup (gmenu_tree_directory_get_name (branch));

  return self;
}

static void
mnb_launcher_directory_free (MnbLauncherDirectory *self)
{
  GSList *iter;

  g_free (self->name);

  iter = self->entries;
  while (iter)
    {
      mnb_launcher_entry_free ((MnbLauncherEntry *) iter->data);
      iter = g_slist_delete_link (iter, iter);
    }

  g_free (self);
}

static gint
mnb_launcher_directory_compare (MnbLauncherDirectory *self,
                                MnbLauncherDirectory *b)
{
  return g_utf8_collate (self->name, b->name);
}

static void
mnb_launcher_directory_sort_entries (MnbLauncherDirectory *self)
{
  self->entries = g_slist_sort (self->entries,
                                (GCompareFunc) mnb_launcher_entry_compare);
}

/*
 * gmenu functions derived from/inspired by gnome-panel, LGPLv2 or later.
 */

GSList * get_all_applications_from_dir (GMenuTreeDirectory *branch,
					                              GSList             *tree,
					                              gboolean            is_root);

static GSList *
get_all_applications_from_alias (GMenuTreeAlias *alias,
                                 GSList         *tree)
{
  GMenuTreeItem         *aliased_item;
  MnbLauncherDirectory  *directory;
  GSList                *ret;

  g_return_val_if_fail (tree, NULL);

  aliased_item = gmenu_tree_alias_get_item (alias);
  directory = (MnbLauncherDirectory *) tree->data;
  ret = tree;

  switch (gmenu_tree_item_get_type (aliased_item))
    {
    case GMENU_TREE_ITEM_ENTRY:
      directory->entries = g_slist_prepend (directory->entries,
                                            mnb_launcher_entry_new (GMENU_TREE_ENTRY (aliased_item)));
      break;

    case GMENU_TREE_ITEM_DIRECTORY:
      ret = get_all_applications_from_dir (
              GMENU_TREE_DIRECTORY (aliased_item),
              tree, FALSE);
      break;

    default:
      break;
  }

  gmenu_tree_item_unref (aliased_item);
  return ret;
}

GSList *
get_all_applications_from_dir (GMenuTreeDirectory *branch,
                               GSList             *tree,
                               gboolean            is_root)
{
  MnbLauncherDirectory  *directory;
  GSList                *list, *iter;
  GSList                *ret;

  directory = NULL;
  ret = tree;
  if (!is_root)
    {
      ret = g_slist_prepend (tree, mnb_launcher_directory_new (branch));
      directory = (MnbLauncherDirectory *) ret->data;
    }

  list = gmenu_tree_directory_get_contents (branch);
  for (iter = list; iter; iter = iter->next)
    {
      switch (gmenu_tree_item_get_type (iter->data))
      	{
          case GMENU_TREE_ITEM_ENTRY:
        	  /* Can be NULL for root dir. */
        	  if (directory)
        	    {
                directory->entries =
                    g_slist_prepend (directory->entries,
                                     mnb_launcher_entry_new (GMENU_TREE_ENTRY (iter->data)));
        	    }
        	  break;

        	case GMENU_TREE_ITEM_DIRECTORY:
        	  ret = get_all_applications_from_dir (
        	          GMENU_TREE_DIRECTORY (iter->data),
        	          ret, FALSE);
        	  break;

        	case GMENU_TREE_ITEM_ALIAS:
        	  ret = get_all_applications_from_alias (
        	          GMENU_TREE_ALIAS (iter->data),
        	          ret);
        	  break;

        	default:
        	  break;
      	}
    gmenu_tree_item_unref (iter->data);
  }

  g_slist_free (list);
  return ret;
}

/*
 * MnbLauncherTree.
 */

struct MnbLauncherTree_ {
  GMenuTree *applications;
  GMenuTree *settings;
};

MnbLauncherTree *
mnb_launcher_tree_create (void)
{
  MnbLauncherTree *self;

  self = g_new0 (MnbLauncherTree, 1);

  self->applications = gmenu_tree_lookup ("applications.menu",
                                          GMENU_TREE_FLAGS_NONE);
  self->settings = gmenu_tree_lookup ("settings.menu",
                                      GMENU_TREE_FLAGS_NONE);
  return self;
}

GSList *
mnb_launcher_tree_list_entries (MnbLauncherTree *self)
{
  GMenuTreeDirectory  *root;
  GSList              *tree;
  GSList              *tree_iter;

  g_return_val_if_fail (tree, NULL);

  tree = NULL;

  /* Applications. */
  root = gmenu_tree_get_root_directory (self->applications);
  tree = get_all_applications_from_dir (root, tree, TRUE);
  gmenu_tree_item_unref (root);

  /* Settings. */
  root = gmenu_tree_get_root_directory (self->settings);
  tree = get_all_applications_from_dir (root, tree, TRUE);
  gmenu_tree_item_unref (root);

  /* Sort directories. */
  tree = g_slist_sort (tree, (GCompareFunc) mnb_launcher_directory_compare);

  /* Sort entries inside directories. */
  for (tree_iter = tree; tree_iter; tree_iter = tree_iter->next)
    {
      mnb_launcher_directory_sort_entries ((MnbLauncherDirectory *) tree_iter->data);
    }

  return tree;
}

MnbLauncherMonitor *
mnb_launcher_tree_create_monitor  (MnbLauncherTree            *tree,
                                   MnbLauncherMonitorFunction  monitor_function,
                                   gpointer                    user_data)
{
  MnbLauncherMonitor *self;

  g_return_val_if_fail (monitor_function, NULL);

  self = g_new0 (MnbLauncherMonitor, 1);

  self->applications = gmenu_tree_ref (tree->applications);
  gmenu_tree_add_monitor (self->applications,
                          (GMenuTreeChangedFunc) applications_menu_changed_cb,
                          self);

  self->settings = gmenu_tree_ref (tree->settings);
  gmenu_tree_add_monitor (self->settings,
                          (GMenuTreeChangedFunc) settings_menu_changed_cb,
                          self);

  self->monitor_function = monitor_function;
  self->user_data = user_data;

  return self;

}

void
mnb_launcher_tree_free_entries (GSList *entries)
{
  GSList *iter;

  iter = entries;
  while (iter)
    {
      mnb_launcher_directory_free ((MnbLauncherDirectory *) iter->data);
      iter = g_slist_delete_link (iter, iter);
    }
}

void
mnb_launcher_tree_free (MnbLauncherTree *tree)
{
  g_return_if_fail (tree);

  gmenu_tree_unref (tree->applications);
  gmenu_tree_unref (tree->settings);

  g_free (tree);
}

const gchar *
mnb_launcher_entry_get_name (MnbLauncherEntry *entry)
{
  const gchar *desktop_file;
  GKeyFile    *key_file = NULL;
  GError      *error = NULL;

  g_return_val_if_fail (entry, NULL);

  /* The generic name is cached. Try to short-cut. */
  if (entry->generic_name)
    return entry->generic_name;

  desktop_file = gmenu_tree_entry_get_desktop_file_path (entry->entry);
  if (!desktop_file)
    goto bail;

  key_file = g_key_file_new ();
  g_key_file_load_from_file (key_file, desktop_file, G_KEY_FILE_NONE, &error);
  if (error)
    {
      g_warning ("%s", error->message);
      goto bail;
    }

  entry->generic_name = g_key_file_get_string (key_file,
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
  if (!entry->generic_name)
    entry->generic_name = g_strdup (gmenu_tree_entry_get_name (entry->entry));

  return entry->generic_name;
}

/*
 * Get executable from menu entry and check it's available in the path.
 * Returns: absolute path if found, otherwise NULL.
 */
gchar *
mnb_launcher_entry_get_exec (MnbLauncherEntry *entry)
{
  const gchar  *exec;
  gint          argc;
  gchar       **argv;
  GError       *error;

  g_return_val_if_fail (entry, NULL);

  exec = gmenu_tree_entry_get_exec (entry->entry);
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

const gchar *
mnb_launcher_entry_get_icon (MnbLauncherEntry *entry)
{
  g_return_val_if_fail (entry, NULL);

  return gmenu_tree_entry_get_icon (entry->entry);
}

const gchar *
mnb_launcher_entry_get_comment (MnbLauncherEntry *entry)
{
  g_return_val_if_fail (entry, NULL);

  return gmenu_tree_entry_get_comment (entry->entry);
}

gchar *
mnb_launcher_utils_get_last_used (const gchar *executable)
{
  struct stat  exec_stat;
  gchar       *last_used;

  if (0 == stat (executable, &exec_stat) &&
      exec_stat.st_atime != exec_stat.st_mtime)
    {
      GTimeVal atime = { 0 ,0 };
      atime.tv_sec = exec_stat.st_atime;
      last_used = penge_utils_format_time (&atime);
    }
  else
    {
      last_used = g_strdup (_("Never opened"));
    }
    
  return last_used;
}


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

#include <glib/gi18n.h>

#define GMENU_I_KNOW_THIS_IS_UNSTABLE
#include <gmenu-tree.h>

#include <penge/penge-utils.h>

#include "mnb-launcher-tree.h"
#include "moblin-netbook.h"

/*
 * Desktop file utils.
 */

static gchar *
desktop_file_get_name (GKeyFile *entry)
{
  gchar *ret = NULL;
  GError *error = NULL;

  g_return_val_if_fail (entry, NULL);

  ret = g_key_file_get_locale_string (entry,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_GENERIC_NAME,
                                      NULL,
                                      &error);
  if (error)
    {
      /* Missing generic name is very common, so just ignore.
       * g_warning ("%s", error->message); */
      g_clear_error (&error);
    }

  if (ret)
    return ret;

  /* Fall back to "Name" */
  ret = g_key_file_get_locale_string (entry,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_NAME,
                                      NULL,
                                      &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_clear_error (&error);
      return NULL;
    }

  return ret;
}

/*
 * Get executable from menu entry and check it's available in the path.
 * Returns: absolute path if found, otherwise NULL.
 */
static gchar *
desktop_file_get_exec (GKeyFile *entry)
{
  gchar   *exec = NULL;
  gchar   *binary = NULL;
  gint     argc;
  gchar  **argv;
  GError  *error = NULL;

  g_return_val_if_fail (entry, NULL);

  error = NULL;
  exec = g_key_file_get_value (entry,
                               G_KEY_FILE_DESKTOP_GROUP,
                               G_KEY_FILE_DESKTOP_KEY_EXEC, &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  if (exec)
    {
      error = NULL;
      if (g_shell_parse_argv (exec, &argc, &argv, &error))
        {
          binary = g_find_program_in_path (argv[0]);
          g_strfreev (argv);
        }
      else
        {
          g_warning ("%s", error->message);
          g_error_free (error);
        }
      g_free (exec);
    }

  return binary;
}

static gchar *
desktop_file_get_icon (GKeyFile *entry)
{
  gchar *ret = NULL;
  GError *error = NULL;

  g_return_val_if_fail (entry, NULL);

  ret = g_key_file_get_locale_string (entry,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_ICON,
                                      NULL,
                                      &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  return ret;
}

static gchar *
desktop_file_get_comment (GKeyFile *entry)
{
  gchar *ret = NULL;
  GError *error = NULL;

  g_return_val_if_fail (entry, NULL);

  ret = g_key_file_get_locale_string (entry,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_COMMENT,
                                      NULL,
                                      &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  return ret;
}

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
  gchar     *desktop_file_path;
  gchar     *name;
  gchar     *exec;
  gchar     *icon;
  gchar     *comment;
};

MnbLauncherEntry *
mnb_launcher_entry_create (const gchar *desktop_file_path)
{
  MnbLauncherEntry  *self;
  GKeyFile          *desktop_file;
  GError            *error;

  g_return_val_if_fail (desktop_file_path, NULL);
  g_return_val_if_fail (*desktop_file_path, NULL);

  desktop_file = g_key_file_new ();
  error = NULL;
  g_key_file_load_from_file (desktop_file,
                             desktop_file_path,
                             G_KEY_FILE_NONE,
                             &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      self = NULL;
    }
  else
    {
      self = g_new0 (MnbLauncherEntry, 1);
      self->desktop_file_path = g_strdup (desktop_file_path);
      self->name = desktop_file_get_name (desktop_file);
      self->exec = desktop_file_get_exec (desktop_file);
      self->icon = desktop_file_get_icon (desktop_file);
      self->comment = desktop_file_get_comment (desktop_file);
    }

  g_key_file_free (desktop_file);

  return self;
}

static MnbLauncherEntry *
mnb_launcher_entry_create_from_gmenu_entry (GMenuTreeEntry *entry)
{
  MnbLauncherEntry *self;

  g_return_val_if_fail (entry, NULL);

  /* We have a patch to libgnome-menu that adds an accessor for the
   * GenericName desktop entry field. */
#if GMENU_WITH_GENERIC_NAME
  self = g_new0 (MnbLauncherEntry, 1);
  self->desktop_file_path = g_strdup (gmenu_tree_entry_get_desktop_file_path (entry));
  self->name = gmenu_tree_entry_get_generic_name (entry) ?
                g_strdup (gmenu_tree_entry_get_generic_name (entry)) :
                g_strdup (gmenu_tree_entry_get_name (entry));
  self->exec = g_strdup (gmenu_tree_entry_get_exec (entry));
  self->icon = g_strdup (gmenu_tree_entry_get_icon (entry));
  self->comment = g_strdup (gmenu_tree_entry_get_comment (entry));
#else
  self = mnb_launcher_entry_create (gmenu_tree_entry_get_desktop_file_path (entry));
#endif

  return self;
}

void
mnb_launcher_entry_free (MnbLauncherEntry *self)
{
  g_return_if_fail (self);

  g_free (self->desktop_file_path);
  g_free (self->name);
  g_free (self->exec);
  g_free (self->icon);
  g_free (self->comment);
  g_free (self);
}

const gchar *
mnb_launcher_entry_get_desktop_file_path (MnbLauncherEntry *entry)
{
  g_return_val_if_fail (entry, NULL);

  return entry->desktop_file_path;
}

const gchar *
mnb_launcher_entry_get_name (MnbLauncherEntry *entry)
{
  g_return_val_if_fail (entry, NULL);

  return entry->name;
}

const gchar *
mnb_launcher_entry_get_exec (MnbLauncherEntry *entry)
{
  g_return_val_if_fail (entry, NULL);

  return entry->exec;
}
const gchar *
mnb_launcher_entry_get_icon (MnbLauncherEntry *entry)
{
  g_return_val_if_fail (entry, NULL);

  return entry->icon;
}

const gchar *
mnb_launcher_entry_get_comment (MnbLauncherEntry *entry)
{
  g_return_val_if_fail (entry, NULL);

  return entry->comment;
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
      directory->entries = g_slist_prepend (
        directory->entries,
        mnb_launcher_entry_create_from_gmenu_entry (GMENU_TREE_ENTRY (aliased_item)));
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
                    g_slist_prepend (
                      directory->entries,
                      mnb_launcher_entry_create_from_gmenu_entry (GMENU_TREE_ENTRY (iter->data)));
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

  g_return_val_if_fail (self, NULL);

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


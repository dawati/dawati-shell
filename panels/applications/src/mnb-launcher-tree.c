/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#define GMENU_I_KNOW_THIS_IS_UNSTABLE
#include <gmenu-tree.h>

#include "mnb-launcher-tree.h"

#define MNB_LAUNCHER_TREE_CACHE_FILE "meego-launcher-cache"

/* Set this to include the settings menu.
  #define MNB_LAUNCHER_TREE_LOAD_SETTINGS
 */

/*
 * MnbLauncherMonitor.
 */

struct MnbLauncherMonitor_ {
  GMenuTree                   *applications;
  GMenuTree                   *settings;
  GSList                      *monitors;
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

#ifdef MNB_LAUNCHER_TREE_LOAD_SETTINGS

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

#endif /* MNB_LAUNCHER_TREE_LOAD_SETTINGS */

static void
applications_directory_changed_cb (GFileMonitor       *monitor,
                                   GFile              *file,
                                   GFile              *other_file,
                                   GFileMonitorEvent   event_type,
                                   MnbLauncherMonitor *self)
{
  /* Filter multiple notifications. */
  g_idle_remove_by_data (self);
  g_idle_add_full (G_PRIORITY_LOW,
                   (GSourceFunc) menu_changed_idle_cb,
                   self, NULL);
}

void
mnb_launcher_monitor_free (MnbLauncherMonitor *self)
{
  GSList *iter;

  g_return_if_fail (self);

  gmenu_tree_remove_monitor (self->applications,
                             (GMenuTreeChangedFunc) applications_menu_changed_cb,
                             self);
  gmenu_tree_unref (self->applications);

#ifdef MNB_LAUNCHER_TREE_LOAD_SETTINGS
  gmenu_tree_remove_monitor (self->settings,
                              (GMenuTreeChangedFunc) settings_menu_changed_cb,
                              self);
  gmenu_tree_unref (self->settings);
#endif /* MNB_LAUNCHER_TREE_LOAD_SETTINGS */

  iter = self->monitors;
  while (iter)
    {
      g_object_unref (G_FILE_MONITOR (iter->data));
      iter = g_slist_delete_link (iter, iter);
    }

  g_free (self);
}

/*
 * MnbLauncherApplication helpers.
 */

static MnbLauncherApplication *
mnb_launcher_application_create_from_gmenu_entry (GMenuTreeEntry *entry)
{
  MnbLauncherApplication *self = NULL;
  int       argc;
  char    **argv;
  GError   *error = NULL;

  g_return_val_if_fail (entry, NULL);

  if (!g_shell_parse_argv (gmenu_tree_entry_get_exec (entry), &argc, &argv, &error))
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
  else if (argc > 0)
    {
      self = mnb_launcher_application_new (gmenu_tree_entry_get_name (entry),
                                           gmenu_tree_entry_get_icon (entry),
                                           gmenu_tree_entry_get_comment (entry),
                                           argv[0],
                                           gmenu_tree_entry_get_desktop_file_path (entry));

      g_strfreev (argv);
    }

  return self;
}

static gint
mnb_launcher_application_compare (MnbLauncherApplication *self,
                            MnbLauncherApplication *b)
{
  return g_utf8_collate (mnb_launcher_application_get_name (self),
                         mnb_launcher_application_get_name (b));
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
  self->id = g_strdup (gmenu_tree_directory_get_menu_id (branch));
  self->name = g_strdup (gmenu_tree_directory_get_name (branch));

  return self;
}

static MnbLauncherDirectory *
mnb_launcher_directory_new_from_cache (const gchar **attribute_names,
                                       const gchar **attribute_values)
{
  MnbLauncherDirectory *self;
  guint i;
  gboolean have_id = FALSE;
  gboolean have_name = FALSE;;

  g_return_val_if_fail (attribute_names, NULL);
  g_return_val_if_fail (attribute_values, NULL);

  self = g_new0 (MnbLauncherDirectory, 1);

  for (i = 0; attribute_names[i]; i++)
    {
      if (0 == g_strcmp0 (attribute_names[i], "id"))
        {
          self->id = g_strdup (attribute_values[i]);
          have_id = TRUE;
        }
      else if (0 == g_strcmp0 (attribute_names[i], "name"))
        {
          self->name = g_strdup (attribute_values[i]);
          have_name = TRUE;
        }
    }

  g_warn_if_fail (have_id && have_name);

  return self;
}

static void
mnb_launcher_directory_free (MnbLauncherDirectory *self)
{
  GList *iter;

  g_free (self->id);
  g_free (self->name);

  iter = self->entries;
  while (iter)
    {
      g_object_unref (MNB_LAUNCHER_APPLICATION (iter->data));
      iter = g_list_delete_link (iter, iter);
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
  self->entries = g_list_sort (self->entries,
                                (GCompareFunc) mnb_launcher_application_compare);
}

static void
mnb_launcher_directory_write_xml (MnbLauncherDirectory const  *self,
                                  FILE                        *fp)
{
  GList const *app_iter;
  gchar       *text;

  text = g_markup_printf_escaped ("  <category id=\"%s\" name=\"%s\">\n",
                                   self->id,
                                   self->name);
  fputs (text, fp);
  g_free (text);
  for (app_iter = self->entries;
       app_iter;
       app_iter = app_iter->next)
    {
      mnb_launcher_application_write_xml ((MnbLauncherApplication const *) app_iter->data,
                                          fp);
    }
  fputs ("  </category>\n", fp);
}

/*
 * gmenu functions derived from/inspired by gnome-panel, LGPLv2 or later.
 */

GList * get_all_applications_from_dir (GMenuTreeDirectory *branch,
					                             GList              *tree,
					                             gboolean            is_root);

static GList *
get_all_applications_from_alias (GMenuTreeAlias *alias,
                                 GList          *tree)
{
  GMenuTreeItem         *aliased_item;
  MnbLauncherDirectory  *directory;
  GList                 *ret;

  g_return_val_if_fail (tree, NULL);

  aliased_item = gmenu_tree_alias_get_item (alias);
  directory = (MnbLauncherDirectory *) tree->data;
  ret = tree;

  switch (gmenu_tree_item_get_type (aliased_item))
    {
    case GMENU_TREE_ITEM_ENTRY:
      directory->entries = g_list_prepend (
        directory->entries,
        mnb_launcher_application_create_from_gmenu_entry (GMENU_TREE_ENTRY (aliased_item)));
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

GList *
get_all_applications_from_dir (GMenuTreeDirectory *branch,
                               GList              *tree,
                               gboolean            is_root)
{
  MnbLauncherDirectory  *directory;
  GSList                *list, *iter;
  GList                 *ret;

  directory = NULL;
  ret = tree;
  if (!is_root)
    {
      ret = g_list_prepend (tree, mnb_launcher_directory_new (branch));
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
                    g_list_prepend (
                      directory->entries,
                      mnb_launcher_application_create_from_gmenu_entry (GMENU_TREE_ENTRY (iter->data)));
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
  GSList    *watch_list;
};

typedef enum {
  ACCUMULATOR_STATE_INVALID = 0,
  ACCUMULATOR_STATE_CACHE,
  ACCUMULATOR_STATE_CATEGORY,
  ACCUMULATOR_STATE_APPLICATION,
  ACCUMULATOR_STATE_APPLICATION_NAME,
  ACCUMULATOR_STATE_APPLICATION_EXECUTABLE,
  ACCUMULATOR_STATE_APPLICATION_ICON,
  ACCUMULATOR_STATE_APPLICATION_DESCRIPTION
} AccumulatorState;

typedef struct {
  GList             *categories;
  GSList            *watch_list;
  AccumulatorState   state;
} Accumulator;

MnbLauncherTree *
mnb_launcher_tree_create (void)
{
  MnbLauncherTree *self;

  self = g_new0 (MnbLauncherTree, 1);

  self->applications = gmenu_tree_lookup ("applications.menu",
                                          GMENU_TREE_FLAGS_NONE);

#ifdef MNB_LAUNCHER_TREE_LOAD_SETTINGS
  self->settings = gmenu_tree_lookup ("settings.menu",
                                      GMENU_TREE_FLAGS_NONE);
#endif

  return self;
}

void
mnb_launcher_tree_free_entries (GList *entries)
{
  GList *iter;

  iter = entries;
  while (iter)
    {
      mnb_launcher_directory_free ((MnbLauncherDirectory *) iter->data);
      iter = g_list_delete_link (iter, iter);
    }
}

static void
mnb_launcher_tree_free_watch_list (GSList *watch_list)
{
  GSList *iter;

  iter = watch_list;
  while (iter)
    {
      g_free (iter->data);
      iter = g_slist_delete_link (iter, iter);
    }
}

void
mnb_launcher_tree_free (MnbLauncherTree *self)
{
  g_return_if_fail (self);

  gmenu_tree_unref (self->applications);

#ifdef MNB_LAUNCHER_TREE_LOAD_SETTINGS
  gmenu_tree_unref (self->settings);
#endif /* MNB_LAUNCHER_TREE_LOAD_SETTINGS */

  mnb_launcher_tree_free_watch_list (self->watch_list);

  g_free (self);
}

static void
mnb_launcher_tree_write_xml (GList const *tree,
                             FILE        *fp)
{
  GList const *directory_iter;

  fputs ("<cache>\n", fp);
  for (directory_iter = tree;
       directory_iter;
       directory_iter = directory_iter->next)
    {
      mnb_launcher_directory_write_xml ((MnbLauncherDirectory const *) directory_iter->data,
                                        fp);
    }
  fputs ("</cache>\n", fp);
}

static GSList *
mnb_launcher_tree_watch_list_add_watch_dir (GSList      *watch_list,
                                            const gchar *dir)
{
  GSList const *iter;

  /* We should at most have half a dozen dirs, so sequential check is fine. */
  for (iter = watch_list; iter; iter = iter->next)
    {
      if (0 == g_strcmp0 (dir, (const gchar *) iter->data))
        {
          /* Known dir. */
          return watch_list;
        }
    }

  return g_slist_prepend (watch_list, g_strdup (dir));
}

static GList *
mnb_launcher_tree_list_categories_from_disk (MnbLauncherTree *self)
{
  GMenuTreeDirectory  *root;
  GList               *tree;
  GList               *tree_iter;

  g_return_val_if_fail (self, NULL);

  tree = NULL;

  /* Applications. */
  root = gmenu_tree_get_root_directory (self->applications);
  tree = get_all_applications_from_dir (root, tree, TRUE);
  gmenu_tree_item_unref (root);

  /* Settings. */
#ifdef MNB_LAUNCHER_TREE_LOAD_SETTINGS
  root = gmenu_tree_get_root_directory (self->settings);
  tree = get_all_applications_from_dir (root, tree, TRUE);
  gmenu_tree_item_unref (root);
#endif

  /* Sort directories. */
  tree = g_list_sort (tree, (GCompareFunc) mnb_launcher_directory_compare);

  /* Sort entries inside directories. */
  for (tree_iter = tree; tree_iter; tree_iter = tree_iter->next)
    {
      mnb_launcher_directory_sort_entries ((MnbLauncherDirectory *) tree_iter->data);
    }

  return tree;
}

static void
_cache_parser_start_element (GMarkupParseContext *context,
                             const gchar         *element_name,
                             const gchar        **attribute_names,
                             const gchar        **attribute_values,
                             gpointer             user_data,
                             GError             **error)
{
  Accumulator *self = (Accumulator *) user_data;

  switch (self->state)
    {
      case ACCUMULATOR_STATE_INVALID:
        if (0 == g_strcmp0 ("cache", element_name))
          {
            self->state = ACCUMULATOR_STATE_CACHE;
            return;
          }
        break;
      case ACCUMULATOR_STATE_CACHE:
        if (0 == g_strcmp0 ("category", element_name))
          {
            MnbLauncherDirectory *cat
              = mnb_launcher_directory_new_from_cache (attribute_names,
                                                       attribute_values);
            if (cat)
              {
                self->categories = g_list_prepend (self->categories, cat);
                self->state = ACCUMULATOR_STATE_CATEGORY;
                return;
              }
          }
        break;
      case ACCUMULATOR_STATE_CATEGORY:
        if (0 == g_strcmp0 ("application", element_name))
          {
            MnbLauncherApplication  *app;
            MnbLauncherDirectory    *cat;

            g_return_if_fail (self->categories);
            g_return_if_fail (self->categories->data);

            cat = (MnbLauncherDirectory *) self->categories->data;
            app = mnb_launcher_application_new_from_cache (attribute_names,
                                                           attribute_values);
            if (app)
              {
                const gchar *desktop_file = mnb_launcher_application_get_desktop_file (app);
                if (desktop_file == NULL)
                  g_warning ("%s Missing desktop file", G_STRLOC);
                else {
                  gchar *dir = g_path_get_dirname (desktop_file);
                  self->watch_list = mnb_launcher_tree_watch_list_add_watch_dir (
                                      self->watch_list,
                                      dir);
                  g_free (dir);
                }
                cat->entries = g_list_prepend (cat->entries, app);
                self->state = ACCUMULATOR_STATE_APPLICATION;

                return;
              }
          }
        break;
      case ACCUMULATOR_STATE_APPLICATION:
        if (0 == g_strcmp0 ("name", element_name))
          {
            self->state = ACCUMULATOR_STATE_APPLICATION_NAME;
            return;
          }
        else if (0 == g_strcmp0 ("executable", element_name))
          {
            self->state = ACCUMULATOR_STATE_APPLICATION_EXECUTABLE;
            return;
          }
        else if (0 == g_strcmp0 ("icon", element_name))
          {
            self->state = ACCUMULATOR_STATE_APPLICATION_ICON;
            return;
          }
        else if (0 == g_strcmp0 ("description", element_name))
          {
            self->state = ACCUMULATOR_STATE_APPLICATION_DESCRIPTION;
            return;
          }
        break;
      /* Fall thru. */
      case ACCUMULATOR_STATE_APPLICATION_NAME:
      case ACCUMULATOR_STATE_APPLICATION_EXECUTABLE:
      case ACCUMULATOR_STATE_APPLICATION_ICON:
      case ACCUMULATOR_STATE_APPLICATION_DESCRIPTION:
        ;
    }

    g_set_error (error, 0, 0, "%s Unknown element '%s' in state '%d'",
                 G_STRLOC,
                 element_name,
                 self->state);
}

static void
_cache_parser_end_element (GMarkupParseContext *context,
                           const gchar         *element_name,
                           gpointer             user_data,
                           GError             **error)
{
  Accumulator *self = (Accumulator *) user_data;

  switch (self->state)
    {
      case ACCUMULATOR_STATE_INVALID:
        /* Break to error. */
        break;
      case ACCUMULATOR_STATE_CACHE:
        if (0 == g_strcmp0 ("cache", element_name))
          {
            self->state = ACCUMULATOR_STATE_INVALID;
            return;
          }
        break;
      case ACCUMULATOR_STATE_CATEGORY:
        if (0 == g_strcmp0 ("category", element_name))
          {
            MnbLauncherDirectory *cat;

            g_return_if_fail (self->categories);
            g_return_if_fail (self->categories->data);

            cat = (MnbLauncherDirectory *) self->categories->data;
            cat->entries = g_list_reverse (cat->entries);

            self->state = ACCUMULATOR_STATE_CACHE;
            return;
          }
        break;
      case ACCUMULATOR_STATE_APPLICATION:
        if (0 == g_strcmp0 ("application", element_name))
          {
            self->state = ACCUMULATOR_STATE_CATEGORY;
            return;
          }
        break;
      case ACCUMULATOR_STATE_APPLICATION_NAME:
        if (0 == g_strcmp0 ("name", element_name))
          {
            self->state = ACCUMULATOR_STATE_APPLICATION;
            return;
          }
        break;
      case ACCUMULATOR_STATE_APPLICATION_EXECUTABLE:
        if (0 == g_strcmp0 ("executable", element_name))
          {
            self->state = ACCUMULATOR_STATE_APPLICATION;
            return;
          }
        break;
      case ACCUMULATOR_STATE_APPLICATION_ICON:
        if (0 == g_strcmp0 ("icon", element_name))
          {
            self->state = ACCUMULATOR_STATE_APPLICATION;
            return;
          }
        break;
      case ACCUMULATOR_STATE_APPLICATION_DESCRIPTION:
        if (0 == g_strcmp0 ("description", element_name))
          {
            self->state = ACCUMULATOR_STATE_APPLICATION;
            return;
          }
        break;
    }

    g_set_error (error, 0, 0, "%s Unknown element '%s' in state '%d'",
                 G_STRLOC,
                 element_name,
                 self->state);
}

static void
_cache_parser_text (GMarkupParseContext *context,
                    const gchar         *text,
                    gsize                text_len,
                    gpointer             user_data,
                    GError             **error)
{
  Accumulator *self = (Accumulator *) user_data;

  switch (self->state)
    {
      case ACCUMULATOR_STATE_INVALID:
        /* Ignore text here. */
        return;
      case ACCUMULATOR_STATE_CACHE:
        /* Ignore text here. */
        return;
      case ACCUMULATOR_STATE_CATEGORY:
        /* Ignore text here. */
        return;
      case ACCUMULATOR_STATE_APPLICATION:
        /* Ignore text here. */
        return;
      /* Fall thru. */
      case ACCUMULATOR_STATE_APPLICATION_NAME:
      case ACCUMULATOR_STATE_APPLICATION_EXECUTABLE:
      case ACCUMULATOR_STATE_APPLICATION_ICON:
      case ACCUMULATOR_STATE_APPLICATION_DESCRIPTION:
        {
          MnbLauncherDirectory    *cat;
          MnbLauncherApplication  *app;

          g_return_if_fail (self->categories);
          g_return_if_fail (self->categories->data);
          cat = (MnbLauncherDirectory *) self->categories->data;
          g_return_if_fail (cat->entries);
          g_return_if_fail (cat->entries->data);

          app = (MnbLauncherApplication *) cat->entries->data;
          switch (self->state)
            {
              case ACCUMULATOR_STATE_APPLICATION_NAME:
                mnb_launcher_application_set_name (app, text);
                return;
              case ACCUMULATOR_STATE_APPLICATION_EXECUTABLE:
                mnb_launcher_application_set_executable (app, text);
                return;
              case ACCUMULATOR_STATE_APPLICATION_ICON:
                mnb_launcher_application_set_icon (app, text);
                return;
              case ACCUMULATOR_STATE_APPLICATION_DESCRIPTION:
                mnb_launcher_application_set_description (app, text);
                return;
              default:
                ; /* Shut up compiler, all cases handled in outer switch. */
            }
        }
    }
}

static GList *
mnb_launcher_tree_list_categories_from_cache (MnbLauncherTree *self,
                                              const gchar     *cache_buffer,
                                              gsize            buffer_size)
{
  /* Accumulator appends to existing watches. */
  Accumulator accumulator = {
    .categories = NULL,
    .watch_list = self->watch_list,
    .state = ACCUMULATOR_STATE_INVALID
  };
  GMarkupParser parser = {
    .start_element = _cache_parser_start_element,
    .end_element   = _cache_parser_end_element,
    .text          = _cache_parser_text,
    .passthrough   = NULL,
    .error         = NULL
  };
  GMarkupParseContext *context = g_markup_parse_context_new (&parser,
                                                             0,
                                                             &accumulator,
                                                             NULL);
  GError *error = NULL;

  g_markup_parse_context_parse (context, cache_buffer, buffer_size, &error);
  if (error)
    {
      g_warning ("%s %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
  g_warn_if_fail (accumulator.state == ACCUMULATOR_STATE_INVALID);

  /* Take over the list of dirs to watch. */
  self->watch_list = accumulator.watch_list;
  accumulator.watch_list = NULL;

  return g_list_reverse (accumulator.categories);
}

static gboolean
mnb_launcher_tree_test_cache (MnbLauncherTree *self,
                              const gchar     *cache_path)
{
  gchar        binary_path[PATH_MAX] = { 0, };
  struct stat  binary_stat;
  struct stat  cache_stat;
  struct stat  watch_stat;
  GSList      *iter;

  /* Do not use cache if we don't have any directories to watch,
   * something must have gone wrong in that case. */
  g_return_val_if_fail (self->watch_list, FALSE);

  if (0 != stat (cache_path, &cache_stat))
    {
      g_warning ("%s Could not stat '%s'",
                  G_STRLOC,
                  cache_path);
      return FALSE;
    }

  /* Do not use cache when it's older than this binary,
   * format or something might have changed. */
  if (0 < readlink ("/proc/self/exe", binary_path, PATH_MAX))
    {
      if (0 == stat (binary_path, &binary_stat))
        {
          if (binary_stat.st_mtime > cache_stat.st_mtime)
            {
              g_debug ("%s Cache miss, '%s' more recent",
                       G_STRLOC,
                       binary_path);
              return FALSE;
            }
        }
    }

  for (iter = self->watch_list; iter; iter = iter->next)
    {
      /* Skip non-exist dirs, e.g. "~/.local/share/applications" is being
       * watched even if not existing, so we get notified about the first
       * per user app install.
       * FIXME: can this cause problems when entire dirs are removed and we
       * still hit the cache -- probably not because /usr/share/applications
       * and friends are rather persistant. */
      if (g_file_test (iter->data, G_FILE_TEST_IS_DIR) &&
          0 == stat ((const gchar *) iter->data, &watch_stat))
        {
          if (watch_stat.st_mtime > cache_stat.st_mtime)
            {
              g_debug ("%s Cache miss, '%s' more recent",
                       G_STRLOC,
                       (const gchar *) iter->data);
              return FALSE;
            }
        }
      else
        {
          g_warning ("%s Could not stat '%s'",
                      G_STRLOC,
                      (const gchar *) iter->data);
        }
    }

  return TRUE;
}

GList *
mnb_launcher_tree_list_entries (MnbLauncherTree *self)
{
  const gchar *cache_dir = g_get_user_cache_dir ();
  gchar       *cache_file = g_strdup_printf ("%s.%s.xml",
                                             MNB_LAUNCHER_TREE_CACHE_FILE,
                                             g_getenv ("LANG"));
  gchar       *cache_path = g_build_filename (cache_dir,
                                              cache_file,
                                              NULL);
  gchar *cache_buffer = NULL;
  gsize  buffer_size;
  GList *tree = NULL;
  gchar *user_apps_dir = NULL;

  /* Avoid duplicate watches. */
  mnb_launcher_tree_free_watch_list (self->watch_list);
  self->watch_list = NULL;

  /* Always watch "~/.local/share/applications" */
  user_apps_dir = g_build_filename (g_get_user_data_dir (),
                                    "applications", NULL);
  self->watch_list = mnb_launcher_tree_watch_list_add_watch_dir (self->watch_list,
                                                                 user_apps_dir);
  g_free (user_apps_dir);

  if (!g_file_test (cache_dir, G_FILE_TEST_IS_DIR))
    {
      g_mkdir (cache_dir, 0755);
    }

  if (g_file_get_contents (cache_path, &cache_buffer, &buffer_size, NULL))
    {
      /* This fills the list of watched dirs. */
      tree = mnb_launcher_tree_list_categories_from_cache (self,
                                                           cache_buffer,
                                                           buffer_size);
      if (!mnb_launcher_tree_test_cache (self, cache_path))
        {
          mnb_launcher_tree_free_entries (tree);
          tree = NULL;
        }
      else
        {
          g_debug ("%s Cache hit", G_STRLOC);
        }
      g_free (cache_buffer);
    }

  if (tree == NULL)
    {
      FILE *fp;
      g_debug ("%s Cache miss", G_STRLOC);
      tree = mnb_launcher_tree_list_categories_from_disk (self);
      fp = fopen (cache_path, "w");
      if (fp)
        {
          mnb_launcher_tree_write_xml (tree, fp);
          fclose (fp);
        }
      else
        g_warning ("%s Could not open '%s'",
                   G_STRLOC,
                   cache_path);
    }

  g_free (cache_file);
  g_free (cache_path);

  return tree;
}

MnbLauncherMonitor *
mnb_launcher_tree_create_monitor  (MnbLauncherTree            *tree,
                                   MnbLauncherMonitorFunction  monitor_function,
                                   gpointer                    user_data)
{
  MnbLauncherMonitor *self;
  GSList *iter;

  g_return_val_if_fail (monitor_function, NULL);

  self = g_new0 (MnbLauncherMonitor, 1);

  /* gmenu monitors */
  self->applications = gmenu_tree_ref (tree->applications);
  gmenu_tree_add_monitor (self->applications,
                          (GMenuTreeChangedFunc) applications_menu_changed_cb,
                          self);

#ifdef MNB_LAUNCHER_TREE_LOAD_SETTINGS
  self->settings = gmenu_tree_ref (tree->settings);
  gmenu_tree_add_monitor (self->settings,
                          (GMenuTreeChangedFunc) settings_menu_changed_cb,
                          self);
#endif /* MNB_LAUNCHER_TREE_LOAD_SETTINGS */

  /* dir monitors for the cache */
  for (iter = tree->watch_list; iter; iter = iter->next)
    {
      GError        *error = NULL;
      GFile         *file = g_file_new_for_path ((const gchar *) iter->data);
      GFileMonitor  *monitor = g_file_monitor (file, 0, NULL, &error);

      if (error)
        {
          g_warning ("%s %s", G_STRLOC, error->message);
          g_clear_error (&error);
        }
      else
        {
          self->monitors = g_slist_prepend (self->monitors, monitor);
          g_signal_connect (monitor, "changed",
                            G_CALLBACK (applications_directory_changed_cb),
                            self);
        }
      g_object_unref (G_OBJECT (file));
    }

  self->monitor_function = monitor_function;
  self->user_data = user_data;

  return self;

}


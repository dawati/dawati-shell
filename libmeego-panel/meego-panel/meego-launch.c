
/*
 * Copyright (c) 2010 Intel Corporation.
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
 *
 * Author: Rob Staudinger <robsta@linux.intel.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>

#include <meego-panel/mpl-app-launch-context.h>
#include <meego-panel/mpl-app-launches-query.h>
#include <meego-panel/mpl-app-launches-store-priv.h>

int
main (int     argc,
      char  **argv)
{
  char const   *executable = NULL;
  char const   *desktop_file = NULL;
  char const  **files = NULL;
  GOptionEntry _options[] = {
/* g_app_info_create_from_commandline() doesn't do startup-notify
   so we can't support that for now as we hook in there.
    { "executable", 'e', 0, G_OPTION_ARG_STRING, (void **) &executable,
      "Launch <executable>, must not contain path", "<executable>" },
*/
    { "desktop-file", 'd', 0, G_OPTION_ARG_STRING, (void **) &desktop_file,
      "Launch <desktop-file> (URI, path or desktop-id)", "<desktop-file>" },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, (void **) &files,
      "Files to open", "[URIS…]" },
    { NULL }
  };

  GOptionContext  *context;
  GAppInfo        *info = NULL;
  GError          *error = NULL;
  int              ret = EXIT_SUCCESS;

  g_type_init ();

  context = g_option_context_new ("- MeeGo launch tool");
  g_option_context_add_main_entries (context, _options, NULL);
  g_option_context_add_group (context, gtk_get_option_group (true));
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical ("%s\n\t%s", G_STRLOC, error->message);
    ret = EXIT_FAILURE;
  }

  if (executable)
  {
    info = g_app_info_create_from_commandline  (executable,
                                                NULL,
                                                G_APP_INFO_CREATE_NONE,
                                                &error);
    if (error)
    {
      g_critical ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
      ret = EXIT_FAILURE;
    }

  } else if (desktop_file) {

    if (g_str_has_prefix (desktop_file, "file://"))
    {
      /* URI */
      char *path = g_filename_from_uri (desktop_file, NULL, &error);
      if (error)
      {
        g_critical ("%s : %s", G_STRLOC, error->message);
        g_clear_error (&error);
        ret = EXIT_FAILURE;

      } else {

        info = (GAppInfo *) g_desktop_app_info_new_from_filename (path);
        if (NULL == info)
        {
          g_critical ("Desktop file '%s' could not be loaded", desktop_file);
          ret = EXIT_FAILURE;
        }
      }
      g_free (path);

    } else if (g_path_is_absolute (desktop_file)) {

      /* desktop file */
      info = (GAppInfo *) g_desktop_app_info_new_from_filename (desktop_file);
      if (NULL == info)
      {
        g_critical ("Desktop file '%s' could not be loaded", desktop_file);
        ret = EXIT_FAILURE;
      }
    } else {

      /* desktop id */
      info = (GAppInfo *) g_desktop_app_info_new (desktop_file);
      if (NULL == info)
      {
        g_critical ("Desktop file '%s' could not be loaded", desktop_file);
        ret = EXIT_FAILURE;
      }
    }
  } else {

    char *help = g_option_context_get_help (context, true, NULL);
    puts (help);
    g_free (help);
  }

  if (info)
  {
    GList *uris = NULL;

    if (files)
    {
      while (*files)
      {
        uris = g_list_prepend (uris, (char *) *files);
        files++;
      }

      if (uris)
        uris = g_list_reverse (uris);
    }

    g_app_info_launch_uris (info,
                            uris,
                            mpl_app_launch_context_get_default (),
                            &error);
    if (error)
    {
      g_critical ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
      ret = EXIT_FAILURE;
    }

    if (uris)
      g_list_free (uris);

    g_object_unref (info);
  }

  g_option_context_free (context);

  return ret;
}


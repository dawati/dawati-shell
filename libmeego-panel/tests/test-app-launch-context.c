
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
  char const *app_info = NULL;
  GOptionEntry _options[] = {
    { "app-info", 'a', 0, G_OPTION_ARG_STRING, (void **) &app_info,
      "Launch <desktop-file>, needs absolute path", "<desktop-file>" },
    { NULL }
  };

  GOptionContext  *context;
  GError          *error = NULL;
  int              ret = EXIT_SUCCESS;

  g_type_init ();

  context = g_option_context_new ("- Test app launch context");
  g_option_context_add_main_entries (context, _options, NULL);
  g_option_context_add_group (context, gtk_get_option_group (true));
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical ("%s\n\t%s", G_STRLOC, error->message);
    ret = EXIT_FAILURE;
  }

  if (app_info)
  {
    GDesktopAppInfo *info = g_desktop_app_info_new_from_filename (app_info);
    g_app_info_launch (G_APP_INFO (info),
                       NULL,
                       mpl_app_launch_context_get_default (),
                       &error);
    if (error)
    {
      g_critical ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
      ret = EXIT_FAILURE;
    }
    g_object_unref (info);

  } else {

    char *help = g_option_context_get_help (context, true, NULL);
    puts (help);
    g_free (help);
  }

  g_option_context_free (context);

  return ret;
}


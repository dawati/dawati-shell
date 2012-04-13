/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Author: Danielle Madeley <danielle.madeley@collabora.co.uk>
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

#include <string.h>

#include <clutter/clutter.h>
#include <mx/mx.h>

#include "mnb-home-plugins-engine.h"
#include "mnb-home-widget.h"

#include "utils.h"

/* size of a single grid square in pixels */
#define GRID_SQUARE 100
#define BORDER_PADDING 6
/* size of a tile */
#define TILE_SIZE ((GRID_SQUARE + BORDER_PADDING) * 2)

static int
load_plugin (const char *filename)
{
  MnbHomePluginsEngine *engine = mnb_home_plugins_engine_dup ();
  ClutterActor *stage, *table, *stack;
  ClutterActor *widget, *config;
  ClutterActor *edit, *quit;
  DawatiHomePluginsApp *app;
  char *path, *module;
  char *p;

  if (!g_str_has_suffix (filename, ".plugin"))
    {
      g_critical ("Please provide a .plugin file");
      return -1;
    }

  path = g_path_get_dirname (filename);
  module = g_path_get_basename (filename);

  p = strrchr (module, '.');
  g_assert (p != NULL);
  *p = '\0';

  DEBUG ("Add path: '%s'", path);
  DEBUG ("Module: '%s'", module);

  peas_engine_insert_search_path (PEAS_ENGINE (engine), 0, path, NULL);
  app = mnb_home_plugins_engine_create_app (engine, module, "/");

  dawati_home_plugins_app_init (app);

  stage = clutter_stage_get_default ();
  table = mx_table_new ();
  stack = mx_stack_new ();

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), table);
  mx_table_insert_actor_with_properties (MX_TABLE (table), stack, 0, 0,
      "x-fill", TRUE,
      "y-fill", TRUE,
      "column-span", 2,
      NULL);

  clutter_actor_set_size (stack, TILE_SIZE, TILE_SIZE);

  widget = dawati_home_plugins_app_get_widget (app);
  config = dawati_home_plugins_app_get_configuration (app);

  clutter_container_add_actor (CLUTTER_CONTAINER (stack), widget);
  clutter_container_add_actor (CLUTTER_CONTAINER (stack), config);
  clutter_actor_hide (config);

  edit = mx_button_new_with_label ("Edit");
  mx_button_set_is_toggle (MX_BUTTON (edit), TRUE);
  mx_table_insert_actor (MX_TABLE (table), edit, 1, 0);

  quit = mx_button_new_with_label ("Quit");
  mx_table_insert_actor (MX_TABLE (table), quit, 1, 1);

  /* FIXME: is there a better way to do this? */
  g_object_bind_property (table, "width", stage, "width",
      G_BINDING_SYNC_CREATE);
  g_object_bind_property (table, "height", stage, "height",
      G_BINDING_SYNC_CREATE);

  /* bind the edit-mode */
  g_object_bind_property (edit, "toggled", config, "visible",
      G_BINDING_DEFAULT);
  g_object_bind_property (edit, "toggled", widget, "visible",
      G_BINDING_INVERT_BOOLEAN);

  g_signal_connect_swapped (quit, "clicked",
      G_CALLBACK (clutter_main_quit), NULL);

  clutter_actor_show (stage);

  clutter_main ();

  dawati_home_plugins_app_deinit (app);

  g_free (path);
  g_free (module);
  g_object_unref (engine);

  return 0;
}

int
main (int argc,
    char **argv)
{
  GOptionContext *context;
  GError *error = NULL;

  context = g_option_context_new ("- launch a Dawati Home plugin in a window");
  g_option_context_add_group (context, clutter_get_option_group ());

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_critical ("Option parsing failed: %s", error->message);
      g_error_free (error);

      return -1;
    }

  g_option_context_free (context);

  if (argc < 2)
    {
      g_critical ("Must specify a .plugin file to load");
      return -1;
    }
  else if (argc > 2)
    {
      g_critical ("May only specify one .plugin file to load");
      return -1;
    }

  return load_plugin (argv[1]);
}

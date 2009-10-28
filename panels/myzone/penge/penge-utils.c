/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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


#include "penge-utils.h"

#include <string.h>
#include <glib/gi18n.h>

#include <nbtk/nbtk.h>
#include <moblin-panel/mpl-panel-client.h>

#include "penge-grid-view.h"

void
penge_utils_load_stylesheet ()
{
  NbtkStyle *style;
  GError *error = NULL;
  gchar *path;

  path = g_build_filename (THEMEDIR,
                           "mutter-moblin.css",
                           NULL);

  /* register the styling */
  style = nbtk_style_get_default ();

  if (!nbtk_style_load_from_file (style,
                             path,
                             &error))
  {
    g_warning (G_STRLOC ": Error opening style: %s",
               error->message);
    g_clear_error (&error);
  }

  g_free (path);
}

void
penge_utils_signal_activated (ClutterActor *actor)
{
  while (actor)
  {
    if (PENGE_IS_GRID_VIEW (actor))
    {
      g_signal_emit_by_name (actor, "activated", NULL);
      return;
    }

    actor = clutter_actor_get_parent (actor);
  }
}

MplPanelClient *
penge_utils_get_panel_client (ClutterActor *actor)
{
  MplPanelClient *panel_client = NULL;

  while (actor)
  {
    if (PENGE_IS_GRID_VIEW (actor))
    {
      g_object_get (actor,
                    "panel-client",
                    &panel_client,
                    NULL);
      return panel_client;
    }

    actor = clutter_actor_get_parent (actor);
  }

  return NULL;
}


gboolean
penge_utils_launch_for_uri (ClutterActor  *actor,
                            const gchar   *uri)
{
  MplPanelClient *client;

  client = penge_utils_get_panel_client (actor);

  if (!client)
    return FALSE;

  return mpl_panel_client_launch_default_application_for_uri (client, uri);
}

gboolean
penge_utils_launch_for_desktop_file (ClutterActor *actor,
                                     const gchar  *path)
{
  MplPanelClient *client;

  client = penge_utils_get_panel_client (actor);

  if (!client)
    return FALSE;

  return mpl_panel_client_launch_application_from_desktop_file (client,
                                                                path,
                                                                NULL);
}

gboolean
penge_utils_launch_by_command_line (ClutterActor *actor,
                                    const gchar  *command_line)
{
  MplPanelClient *client;

  client = penge_utils_get_panel_client (actor);

  if (!client)
    return FALSE;

  return mpl_panel_client_launch_application (client, command_line);
}


/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <glib.h>
#include <clutter/clutter.h>
#include <moblin-panel/mpl-panel-client.h>

void penge_utils_load_stylesheet (void);
void penge_utils_signal_activated (ClutterActor *actor);

MplPanelClient *penge_utils_get_panel_client (ClutterActor *actor);
gboolean penge_utils_launch_for_uri (ClutterActor *actor,
                                     const gchar  *uri);
gboolean penge_utils_launch_for_desktop_file (ClutterActor *actor,
                                              const gchar  *path);
gboolean penge_utils_launch_by_command_line (ClutterActor *actor,
                                             const gchar  *command_line);

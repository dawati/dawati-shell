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


#include <glib.h>
#include <clutter/clutter.h>
#include <meego-panel/mpl-panel-client.h>

void penge_utils_load_stylesheet (void);
void penge_utils_signal_activated (ClutterActor *actor);

MplPanelClient *penge_utils_get_panel_client (ClutterActor *actor);
gboolean penge_utils_launch_for_uri (ClutterActor *actor,
                                     const gchar  *uri);
gboolean penge_utils_launch_for_desktop_file (ClutterActor *actor,
                                              const gchar  *path);
gboolean penge_utils_launch_by_command_line (ClutterActor *actor,
                                             const gchar  *command_line);
void penge_utils_set_locale (void);

/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *         Thomas Wood <thomas@linux.intel.com>
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

#ifndef MOBLIN_NETBOOK_CHOOSER_H
#define MOBLIN_NETBOOK_CHOOSER_H

#include <gio/gdesktopappinfo.h>
#include "moblin-netbook.h"

void     show_workspace_chooser (MutterPlugin *plugin,
                                 const gchar  *sn_id, guint32 timestamp);
void     hide_workspace_chooser (MutterPlugin *plugin, guint32 timestamp);

void     moblin_netbook_sn_setup (MutterPlugin *plugin);

gboolean moblin_netbook_sn_should_map (MutterPlugin *plugin,
                                       MutterWindow *mcw,
                                       const gchar  *sn_id);

void     moblin_netbook_sn_finalize (MutterPlugin *plugin);

gboolean
moblin_netbook_launch_application_from_info (GAppInfo     *app,
                                             GList        *files,
                                             gint          workspace,
                                             gboolean      no_chooser);

gboolean moblin_netbook_launch_application (const  gchar *path,
                                            gint          workspace,
                                            gboolean      no_chooser);

gboolean moblin_netbook_launch_application_from_desktop_file (const  gchar *desktop,
                                                              GList        *files,
                                                              gint          workspace,
                                                              gboolean      no_chooser);

gboolean moblin_netbook_launch_default_for_uri (const gchar *uri,
                                                gint         workspace,
                                                gboolean     no_chooser);

#endif


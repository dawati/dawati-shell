/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
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

#ifndef MNB_LAUNCHER_TREE_H
#define MNB_LAUNCHER_TREE_H

#include <glib.h>
#include "mnb-launcher-application.h"

G_BEGIN_DECLS

/*
 * MnbLauncherMonitor to monitor menu changes.
 */
typedef struct MnbLauncherMonitor_ MnbLauncherMonitor;

typedef void (*MnbLauncherMonitorFunction) (MnbLauncherMonitor *monitor,
                                            gpointer            user_data);

void mnb_launcher_monitor_free (MnbLauncherMonitor *monitor);

/*
 * MnbLauncherTree represents the entire menu hierarchy.
 */

typedef struct MnbLauncherTree_ MnbLauncherTree;

MnbLauncherTree *     mnb_launcher_tree_create          (void);
GList *               mnb_launcher_tree_list_entries    (MnbLauncherTree            *tree);
MnbLauncherMonitor *  mnb_launcher_tree_create_monitor  (MnbLauncherTree            *tree,
                                                         MnbLauncherMonitorFunction  monitor_function,
                                                         gpointer                    user_data);
void                  mnb_launcher_tree_free_entries    (GList                      *entries);
void                  mnb_launcher_tree_free            (MnbLauncherTree            *tree);

/*
 * MnbLauncherDirectory represents a "folder" item in the main menu.
 */
typedef struct {
  gchar *id;
  gchar *name;
  GList *entries;
} MnbLauncherDirectory;

G_END_DECLS

#endif /* MNB_LAUNCHER_TREE_H */


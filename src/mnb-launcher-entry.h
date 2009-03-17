/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
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

#ifndef MNB_LAUNCHER_ENTRY_H
#define MNB_LAUNCHER_ENTRY_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct MnbLauncherEntry_ MnbLauncherEntry;

typedef struct {
  GSList *head;
} MnbLauncherEntryList;

GHashTable *  mnb_launcher_entry_build_hash   (void);

const gchar * mnb_launcher_entry_get_name     (MnbLauncherEntry *entry);
gchar *       mnb_launcher_entry_get_exec     (MnbLauncherEntry *entry);
const gchar * mnb_launcher_entry_get_icon     (MnbLauncherEntry *entry);
const gchar * mnb_launcher_entry_get_comment  (MnbLauncherEntry *entry);

G_END_DECLS

#endif /* MNB_LAUNCHER_ENTRY_H */


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

/*
 * MnbLauncherEntry represents a "launcher" item in the main menu.
 */

#define MNB_TYPE_LAUNCHER_ENTRY (mnb_launcher_entry_get_type ())

typedef struct MnbLauncherEntry_ MnbLauncherEntry;

struct MnbLauncherEntry_ {
  gchar     *desktop_file_path;
  gchar     *name;
  gchar     *exec;
  gchar     *icon;
  gchar     *comment;
};

GType mnb_launcher_entry_get_type (void);

MnbLauncherEntry *  mnb_launcher_entry_create                 (const gchar *desktop_file_path);
void                mnb_launcher_entry_free                   (MnbLauncherEntry *self);

const gchar *       mnb_launcher_entry_get_name               (MnbLauncherEntry *self);
const gchar *       mnb_launcher_entry_get_exec               (MnbLauncherEntry *self);
const gchar *       mnb_launcher_entry_get_icon               (MnbLauncherEntry *self);
const gchar *       mnb_launcher_entry_get_comment            (MnbLauncherEntry *self);
const gchar *       mnb_launcher_entry_get_desktop_file_path  (MnbLauncherEntry *self);

G_END_DECLS

#endif /* MNB_LAUNCHER_ENTRY_H */


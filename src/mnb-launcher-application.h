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

#ifndef MNB_LAUNCHER_APPLICATION_H
#define MNB_LAUNCHER_APPLICATION_H

#include <glib.h>

G_BEGIN_DECLS

#define MNB_TYPE_LAUNCHER_APPLICATION mnb_launcher_application_get_type()

#define MNB_LAUNCHER_APPLICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MNB_TYPE_LAUNCHER_APPLICATION, MnbLauncherApplication))

#define MNB_LAUNCHER_APPLICATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MNB_TYPE_LAUNCHER_APPLICATION, MnbLauncherApplicationClass))

#define MNB_IS_LAUNCHER_APPLICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MNB_TYPE_LAUNCHER_APPLICATION))

#define MNB_IS_LAUNCHER_APPLICATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MNB_TYPE_LAUNCHER_APPLICATION))

#define MNB_LAUNCHER_APPLICATION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MNB_TYPE_LAUNCHER_APPLICATION, MnbLauncherApplicationClass))

typedef struct MnbLauncherApplication_ MnbLauncherApplication;
typedef struct MnbLauncherApplicationClass_ MnbLauncherApplicationClass;

struct MnbLauncherApplication_ {
  GObject parent;
};

struct MnbLauncherApplicationClass_ {
  GObjectClass parent;
};

GType mnb_launcher_application_get_type (void) G_GNUC_CONST;

MnbLauncherApplication *  mnb_launcher_application_new (const gchar *name,
                                                        const gchar *icon,
                                                        const gchar *description,
                                                        const gchar *executable,
                                                        const gchar *desktop_file);

MnbLauncherApplication *  mnb_launcher_application_new_from_desktop_file  (const gchar *desktop_file);

const gchar *       mnb_launcher_application_get_name               (MnbLauncherApplication *self);
const gchar *       mnb_launcher_application_get_executable         (MnbLauncherApplication *self);
const gchar *       mnb_launcher_application_get_icon               (MnbLauncherApplication *self);
const gchar *       mnb_launcher_application_get_description        (MnbLauncherApplication *self);
const gchar *       mnb_launcher_application_get_desktop_file       (MnbLauncherApplication *self);

gboolean            mnb_launcher_application_get_bookmark           (MnbLauncherApplication *self);

G_END_DECLS

#endif /* MNB_LAUNCHER_APPLICATION_H */


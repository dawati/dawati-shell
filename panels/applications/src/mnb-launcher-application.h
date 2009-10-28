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

#ifndef MNB_LAUNCHER_APPLICATION_H
#define MNB_LAUNCHER_APPLICATION_H

#include <stdio.h>
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

MnbLauncherApplication *  mnb_launcher_application_new_from_cache (const gchar **attribute_names,
                                                                   const gchar **attribute_values);

const gchar *       mnb_launcher_application_get_name               (MnbLauncherApplication *self);
void                mnb_launcher_application_set_name               (MnbLauncherApplication *self,
                                                                     const gchar            *name);
const gchar *       mnb_launcher_application_get_executable         (MnbLauncherApplication *self);
void                mnb_launcher_application_set_executable         (MnbLauncherApplication *self,
                                                                     const gchar            *executable);
const gchar *       mnb_launcher_application_get_icon               (MnbLauncherApplication *self);
void                mnb_launcher_application_set_icon               (MnbLauncherApplication *self,
                                                                     const gchar            *icon);
const gchar *       mnb_launcher_application_get_description        (MnbLauncherApplication *self);
void                mnb_launcher_application_set_description        (MnbLauncherApplication *self,
                                                                     const gchar            *description);
const gchar *       mnb_launcher_application_get_desktop_file       (MnbLauncherApplication *self);
void                mnb_launcher_application_set_desktop_file       (MnbLauncherApplication *self,
                                                                     const gchar            *desktop_file);
gboolean            mnb_launcher_application_get_bookmarked         (MnbLauncherApplication *self);
void                mnb_launcher_application_set_bookmarked         (MnbLauncherApplication *self,
                                                                     gboolean                bookmarked);

void                mnb_launcher_application_write_xml              (MnbLauncherApplication const *self,
                                                                     FILE                         *fp);

G_END_DECLS

#endif /* MNB_LAUNCHER_APPLICATION_H */


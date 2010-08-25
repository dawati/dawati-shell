/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *         Thomas Wood <thomas@linux.intel.com>
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

#ifndef MEEGO_NETBOOK_LAUNCHER_H
#define MEEGO_NETBOOK_LAUNCHER_H

#include <glib.h>
#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MNB_TYPE_LAUNCHER mnb_launcher_get_type()

#define MNB_LAUNCHER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MNB_TYPE_LAUNCHER, MnbLauncher))

#define MNB_LAUNCHER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MNB_TYPE_LAUNCHER, MnbLauncherClass))

#define MNB_IS_LAUNCHER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MNB_TYPE_LAUNCHER))

#define MNB_IS_LAUNCHER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MNB_TYPE_LAUNCHER))

#define MNB_LAUNCHER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MNB_TYPE_LAUNCHER, MnbLauncherClass))

typedef struct MnbLauncher_ MnbLauncher;
typedef struct MnbLauncherClass_ MnbLauncherClass;
typedef struct MnbLauncherPrivate_ MnbLauncherPrivate;

struct MnbLauncher_ {
  MxBoxLayout parent;
  MnbLauncherPrivate  *priv;
};

struct MnbLauncherClass_ {
  MxBoxLayoutClass parent;

  /* Signals. */
  void (* launcher_activated) (MnbLauncher  *self,
                               const gchar  *desktop_file);
};

GType mnb_launcher_get_type (void) G_GNUC_CONST;


ClutterActor *  mnb_launcher_new            (void);
void            mnb_launcher_clear_filter   (MnbLauncher *self);

gboolean        mnb_launcher_get_show_expanders (MnbLauncher *self);

void            mnb_launcher_set_show_expanders (MnbLauncher *self,
                                                 gboolean     show_expanders);

void            mnb_launcher_update_last_launched (MnbLauncher *self);

G_END_DECLS

#endif

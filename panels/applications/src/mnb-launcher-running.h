/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * MnbLauncherRunning - keep track of running applications to match against
 * applications in the dawati application panel
 *
 * Copyright © 2012 Intel Corporation.
 *
 * Author: Michael Wood <michael.g.wood@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */


/* mnb-launcher-running.h */



#ifndef _MNB_LAUNCHER_RUNNING_H
#define _MNB_LAUNCHER_RUNNING_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MNB_TYPE_MNB_LAUNCHER_RUNNING mnb_launcher_running_get_type()

#define MNB_MNB_LAUNCHER_RUNNING(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MNB_TYPE_MNB_LAUNCHER_RUNNING, MnbLauncherRunning))

#define MNB_MNB_LAUNCHER_RUNNING_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MNB_TYPE_MNB_LAUNCHER_RUNNING, MnbLauncherRunningClass))

#define MNB_IS_MNB_LAUNCHER_RUNNING(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MNB_TYPE_MNB_LAUNCHER_RUNNING))

#define MNB_IS_MNB_LAUNCHER_RUNNING_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MNB_TYPE_MNB_LAUNCHER_RUNNING))

#define MNB_MNB_LAUNCHER_RUNNING_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MNB_TYPE_MNB_LAUNCHER_RUNNING, MnbLauncherRunningClass))

typedef struct _MnbLauncherRunning MnbLauncherRunning;
typedef struct _MnbLauncherRunningClass MnbLauncherRunningClass;
typedef struct _MnbLauncherRunningPrivate MnbLauncherRunningPrivate;

struct _MnbLauncherRunning
{
  GObject parent;

  MnbLauncherRunningPrivate *priv;
};

struct _MnbLauncherRunningClass
{
  GObjectClass parent_class;
};

GType mnb_launcher_running_get_type (void) G_GNUC_CONST;


MnbLauncherRunning *mnb_launcher_running_new (void);
GList *mnb_launcher_running_get_running (MnbLauncherRunning *self);
void mnb_launcher_running_free (MnbLauncherRunning *self);

G_END_DECLS

#endif /* _MNB_LAUNCHER_RUNNING_H */

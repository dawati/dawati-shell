/*
 * Copyright (C) 2009 Intel Corporation.
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

#ifndef _MNB_LAUNCHER_BUTTON
#define _MNB_LAUNCHER_BUTTON

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_LAUNCHER_BUTTON mnb_launcher_button_get_type()

#define MNB_LAUNCHER_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButton))

#define MNB_LAUNCHER_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButtonClass))

#define MNB_IS_LAUNCHER_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_LAUNCHER_BUTTON))

#define MNB_IS_LAUNCHER_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_LAUNCHER_BUTTON))

#define MNB_LAUNCHER_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButtonClass))

typedef struct {
  NbtkButton parent;
} MnbLauncherButton;

typedef struct {
  NbtkButtonClass parent_class;
} MnbLauncherButtonClass;

GType mnb_launcher_button_get_type (void);

NbtkWidget *mnb_launcher_button_new (const gchar *desktop_file_name);
gboolean mnb_launcher_button_launch (MnbLauncherButton *button);

G_END_DECLS

#endif /* _MNB_LAUNCHER_BUTTON */


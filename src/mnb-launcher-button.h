/*
 * Copyright (C) 2008 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Robert Staudinger <robsta@o-hand.com>.
 * Based on nbtk-label.h by Thomas Wood <thomas@linux.intel.com>.
 */

#ifndef __MNB_LAUNCHER_BUTTON_H__
#define __MNB_LAUNCHER_BUTTON_H__

#include <nbtk/nbtk-widget.h>
#include <nbtk/nbtk-table.h>

G_BEGIN_DECLS

#define MNB_TYPE_LAUNCHER_BUTTON              (mnb_launcher_button_get_type ())
#define MNB_LAUNCHER_BUTTON(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButton))
#define MNB_IS_LAUNCHER_BUTTON(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_LAUNCHER_BUTTON))
#define MNB_LAUNCHER_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButtonClass))
#define NBTK_IS_LAUNCHER_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_LAUNCHER_BUTTON))
#define MNB_LAUNCHER_BUTTON_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButtonClass))

typedef struct _MnbLauncherButton              MnbLauncherButton;
typedef struct _MnbLauncherButtonPrivate       MnbLauncherButtonPrivate;
typedef struct _MnbLauncherButtonClass         MnbLauncherButtonClass;

struct _MnbLauncherButton
{
  /*< private >*/
  NbtkTable parent;

  MnbLauncherButtonPrivate *priv;
};

struct _MnbLauncherButtonClass
{
  NbtkTableClass parent;

  /* signals */
  void (* activated) (MnbLauncherButton     *self,
                      ClutterButtonEvent    *event);
};

GType mnb_launcher_button_get_type (void) G_GNUC_CONST;

NbtkWidget * mnb_launcher_button_new (const gchar *icon_file,
                                      gint         icon_size,
                                      const gchar *title,
                                      const gchar *category,
                                      const gchar *description,
                                      const gchar *comment,
                                      const gchar *executable);

const char *  mnb_launcher_button_get_title       (MnbLauncherButton *self);
const char *  mnb_launcher_button_get_category    (MnbLauncherButton *self);
const char *  mnb_launcher_button_get_description (MnbLauncherButton *self);
const char *  mnb_launcher_button_get_comment     (MnbLauncherButton *self);
const char *  mnb_launcher_button_get_executable  (MnbLauncherButton *self);

G_END_DECLS

#endif /* __MNB_LAUNCHER_BUTTON_H__ */

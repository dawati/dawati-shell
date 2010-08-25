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

#include <mx/mx.h>

G_BEGIN_DECLS

#define MNB_TYPE_LAUNCHER_BUTTON              (mnb_launcher_button_get_type ())
#define MNB_LAUNCHER_BUTTON(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButton))
#define MNB_IS_LAUNCHER_BUTTON(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_LAUNCHER_BUTTON))
#define MNB_LAUNCHER_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButtonClass))
#define MNB_IS_LAUNCHER_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_LAUNCHER_BUTTON))
#define MNB_LAUNCHER_BUTTON_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButtonClass))

typedef struct _MnbLauncherButton              MnbLauncherButton;
typedef struct _MnbLauncherButtonPrivate       MnbLauncherButtonPrivate;
typedef struct _MnbLauncherButtonClass         MnbLauncherButtonClass;

struct _MnbLauncherButton
{
  /*< private >*/
  MxTable parent;

  MnbLauncherButtonPrivate *priv;
};

struct _MnbLauncherButtonClass
{
  MxTableClass parent;

  /* signals */
  void (* hovered)      (MnbLauncherButton *self);
  void (* activated)    (MnbLauncherButton *self);
  void (* fav_toggled)  (MnbLauncherButton *self);
};

GType mnb_launcher_button_get_type (void) G_GNUC_CONST;

MxWidget * mnb_launcher_button_new (const gchar *icon_name,
                                    const gchar *icon_file,
                                    gint         icon_size,
                                    const gchar *title,
                                    const gchar *category,
                                    const gchar *description,
                                    const gchar *executable,
                                    const gchar *desktop_file_path);

MxWidget *  mnb_launcher_button_create_favorite (MnbLauncherButton *self);

const char *  mnb_launcher_button_get_title       (MnbLauncherButton *self);
const char *  mnb_launcher_button_get_category    (MnbLauncherButton *self);
const char *  mnb_launcher_button_get_description (MnbLauncherButton *self);
const char *  mnb_launcher_button_get_executable  (MnbLauncherButton *self);

const char *  mnb_launcher_button_get_desktop_file_path  (MnbLauncherButton *self);

gboolean      mnb_launcher_button_get_favorite    (MnbLauncherButton *self);
void          mnb_launcher_button_set_favorite    (MnbLauncherButton *self,
                                                   gboolean           is_favorite);

const gchar * mnb_launcher_button_get_icon_name   (MnbLauncherButton  *self);
void          mnb_launcher_button_set_icon        (MnbLauncherButton  *self,
                                                   const gchar        *icon_file,
                                                   gint                icon_size);

void          mnb_launcher_button_set_last_launched (MnbLauncherButton  *self,
                                                     time_t              last_launched);

gint          mnb_launcher_button_compare         (MnbLauncherButton *self,
                                                   MnbLauncherButton *other);

gboolean      mnb_launcher_button_match           (MnbLauncherButton *self,
                                                   const gchar       *lcase_needle);

void          mnb_launcher_button_sync_if_favorite  (MnbLauncherButton *self,
                                                     MnbLauncherButton *other);

void          mnb_launcher_button_make_favorite (MnbLauncherButton *self,
                                                 gfloat             width,
                                                 gfloat             height);

G_END_DECLS

#endif /* __MNB_LAUNCHER_BUTTON_H__ */

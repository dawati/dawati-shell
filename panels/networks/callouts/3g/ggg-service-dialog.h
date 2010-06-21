/*
 * Carrick - a connection panel for the MeeGo Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Ross Burton <ross@linux.intel.com>
 */

#ifndef __GGG_SERVICE_DIALOG_H__
#define __GGG_SERVICE_DIALOG_H__

#include <gtk/gtk.h>
#include "ggg-service.h"

G_BEGIN_DECLS

#define GGG_TYPE_SERVICE_DIALOG (ggg_service_dialog_get_type())
#define GGG_SERVICE_DIALOG(obj)                                         \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GGG_TYPE_SERVICE_DIALOG,                \
                                GggServiceDialog))
#define GGG_SERVICE_DIALOG_CLASS(klass)                                 \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GGG_TYPE_SERVICE_DIALOG,                   \
                             GggServiceDialogClass))
#define IS_GGG_SERVICE_DIALOG(obj)                                      \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GGG_TYPE_SERVICE_DIALOG))
#define IS_GGG_SERVICE_DIALOG_CLASS(klass)                              \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GGG_TYPE_SERVICE_DIALOG))
#define GGG_SERVICE_DIALOG_GET_CLASS(obj)                               \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GGG_TYPE_SERVICE_DIALOG,                 \
                               GggServiceDialogClass))

typedef struct _GggServiceDialogPrivate GggServiceDialogPrivate;
typedef struct _GggServiceDialog      GggServiceDialog;
typedef struct _GggServiceDialogClass GggServiceDialogClass;

struct _GggServiceDialog {
  GtkDialog parent;
  GggServiceDialogPrivate *priv;
};

struct _GggServiceDialogClass {
  GtkDialogClass parent_class;
};

GType ggg_service_dialog_get_type (void) G_GNUC_CONST;

GggService * ggg_service_dialog_get_selected (GggServiceDialog *dialog);

G_END_DECLS

#endif /* __GGG_SERVICE_DIALOG_H__ */

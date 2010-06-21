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

#ifndef __GGG_MANUAL_DIALOG_H__
#define __GGG_MANUAL_DIALOG_H__

#include <gtk/gtk.h>
#include <rest/rest-xml-parser.h>

G_BEGIN_DECLS

#define GGG_TYPE_MANUAL_DIALOG (ggg_manual_dialog_get_type())
#define GGG_MANUAL_DIALOG(obj)                                          \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GGG_TYPE_MANUAL_DIALOG,                 \
                                GggManualDialog))
#define GGG_MANUAL_DIALOG_CLASS(klass)                                  \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GGG_TYPE_MANUAL_DIALOG,                    \
                             GggManualDialogClass))
#define IS_GGG_MANUAL_DIALOG(obj)                                       \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GGG_TYPE_MANUAL_DIALOG))
#define IS_GGG_MANUAL_DIALOG_CLASS(klass)                               \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GGG_TYPE_MANUAL_DIALOG))
#define GGG_MANUAL_DIALOG_GET_CLASS(obj)                                \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GGG_TYPE_MANUAL_DIALOG,                  \
                               GggManualDialogClass))

typedef struct _GggManualDialogPrivate GggManualDialogPrivate;
typedef struct _GggManualDialog      GggManualDialog;
typedef struct _GggManualDialogClass GggManualDialogClass;

struct _GggManualDialog {
  GtkDialog parent;
  GggManualDialogPrivate *priv;
};

struct _GggManualDialogClass {
  GtkDialogClass parent_class;
};

GType ggg_manual_dialog_get_type (void) G_GNUC_CONST;

RestXmlNode * ggg_manual_dialog_get_plan (GggManualDialog *dialog);

G_END_DECLS

#endif /* __GGG_MANUAL_DIALOG_H__ */

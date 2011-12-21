/*
 * Carrick - a connection panel for the Dawati Netbook
 * Copyright (C) 2011 Intel Corporation. All rights reserved.
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
 * Written by - Jussi Kukkonen <jussi.kukkonen@intel.com>
 */

#ifndef __GGG_SIM_DIALOG_H__
#define __GGG_SIM_DIALOG_H__

#include <gtk/gtk.h>
#include <ggg-sim.h>

G_BEGIN_DECLS

#define GGG_TYPE_SIM_DIALOG (ggg_sim_dialog_get_type())
#define GGG_SIM_DIALOG(obj)                                         \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GGG_TYPE_SIM_DIALOG,                \
                                GggSimDialog))
#define GGG_SIM_DIALOG_CLASS(klass)                                 \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GGG_TYPE_SIM_DIALOG,                   \
                             GggSimDialogClass))
#define IS_GGG_SIM_DIALOG(obj)                                      \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GGG_TYPE_SIM_DIALOG))
#define IS_GGG_SIM_DIALOG_CLASS(klass)                              \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GGG_TYPE_SIM_DIALOG))
#define GGG_SIM_DIALOG_GET_CLASS(obj)                               \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GGG_TYPE_SIM_DIALOG,                 \
                               GggSimDialogClass))

typedef struct _GggSimDialogPrivate GggSimDialogPrivate;
typedef struct _GggSimDialog      GggSimDialog;
typedef struct _GggSimDialogClass GggSimDialogClass;

struct _GggSimDialog {
  GtkDialog parent;
  GggSimDialogPrivate *priv;
};

struct _GggSimDialogClass {
  GtkDialogClass parent_class;
};

GType ggg_sim_dialog_get_type (void) G_GNUC_CONST;

Sim *ggg_sim_dialog_get_selected (GggSimDialog *dialog);

G_END_DECLS

#endif /* __GGG_SIM_DIALOG_H__ */

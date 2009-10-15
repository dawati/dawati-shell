/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Emmanuele Bassi <ebassi@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MNB_IM_STATUS_ROW_H__
#define __MNB_IM_STATUS_ROW_H__

#include <nbtk/nbtk.h>
#include <telepathy-glib/util.h>

G_BEGIN_DECLS

#define MNB_TYPE_IM_STATUS_ROW                   (mnb_im_status_row_get_type ())
#define MNB_IM_STATUS_ROW(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_IM_STATUS_ROW, MnbIMStatusRow))
#define MNB_IS_IM_STATUS_ROW(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_IM_STATUS_ROW))
#define MNB_IM_STATUS_ROW_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_IM_STATUS_ROW, MnbIMStatusRowClass))
#define MNB_IS_IM_STATUS_ROW_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_IM_STATUS_ROW))
#define MNB_IM_STATUS_ROW_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_IM_STATUS_ROW, MnbIMStatusRowClass))

typedef struct _MnbIMStatusRow          MnbIMStatusRow;
typedef struct _MnbIMStatusRowPrivate   MnbIMStatusRowPrivate;
typedef struct _MnbIMStatusRowClass     MnbIMStatusRowClass;

struct _MnbIMStatusRow
{
  NbtkWidget parent_instance;

  MnbIMStatusRowPrivate *priv;
};

struct _MnbIMStatusRowClass
{
  NbtkWidgetClass parent_class;
};

GType mnb_im_status_row_get_type (void);

NbtkWidget *mnb_im_status_row_new        (const gchar              *account_name);

void        mnb_im_status_row_set_online (MnbIMStatusRow           *row,
                                          gboolean                  is_online);
void        mnb_im_status_row_set_enabled (MnbIMStatusRow           *row,
                                           gboolean                  is_enabled);
void        mnb_im_status_row_set_status (MnbIMStatusRow           *row,
                                          TpConnectionPresenceType  presence,
                                          const gchar              *status);
void        mnb_im_status_row_update     (MnbIMStatusRow           *row);

G_END_DECLS

#endif /* __MNB_IM_STATUS_ROW_H__ */

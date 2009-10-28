/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Emmanuele Bassi <ebassi@linux.intel.com>
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

#ifndef __MNB_WEB_STATUS_ROW_H__
#define __MNB_WEB_STATUS_ROW_H__

#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_WEB_STATUS_ROW                   (mnb_web_status_row_get_type ())
#define MNB_WEB_STATUS_ROW(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_WEB_STATUS_ROW, MnbWebStatusRow))
#define MNB_IS_WEB_STATUS_ROW(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_WEB_STATUS_ROW))
#define MNB_WEB_STATUS_ROW_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_WEB_STATUS_ROW, MnbWebStatusRowClass))
#define MNB_IS_WEB_STATUS_ROW_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_WEB_STATUS_ROW))
#define MNB_WEB_STATUS_ROW_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_WEB_STATUS_ROW, MnbWebStatusRowClass))

typedef struct _MnbWebStatusRow          MnbWebStatusRow;
typedef struct _MnbWebStatusRowPrivate   MnbWebStatusRowPrivate;
typedef struct _MnbWebStatusRowClass     MnbWebStatusRowClass;

struct _MnbWebStatusRow
{
  NbtkWidget parent_instance;

  MnbWebStatusRowPrivate *priv;
};

struct _MnbWebStatusRowClass
{
  NbtkWidgetClass parent_class;
};

GType mnb_web_status_row_get_type (void);

NbtkWidget *mnb_web_status_row_new (const gchar *service_name);

void mnb_web_status_row_force_update (MnbWebStatusRow *row);

G_END_DECLS

#endif /* __MNB_WEB_STATUS_ROW_H__ */

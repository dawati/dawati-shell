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

#ifndef __MNB_CLIPBOARD_VIEW_H__
#define __MNB_CLIPBOARD_VIEW_H__

#include <nbtk/nbtk.h>
#include "mnb-clipboard-store.h"

G_BEGIN_DECLS

#define MNB_TYPE_CLIPBOARD_VIEW                 (mnb_clipboard_view_get_type ())
#define MNB_CLIPBOARD_VIEW(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_CLIPBOARD_VIEW, MnbClipboardView))
#define MNB_IS_CLIPBOARD_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_CLIPBOARD_VIEW))
#define MNB_CLIPBOARD_VIEW_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_CLIPBOARD_VIEW, MnbClipboardViewClass))
#define MNB_IS_CLIPBOARD_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_CLIPBOARD_VIEW))
#define MNB_CLIPBOARD_VIEW_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_CLIPBOARD_VIEW, MnbClipboardViewClass))

typedef struct _MnbClipboardView                MnbClipboardView;
typedef struct _MnbClipboardViewPrivate         MnbClipboardViewPrivate;
typedef struct _MnbClipboardViewClass           MnbClipboardViewClass;

struct _MnbClipboardView
{
  NbtkWidget parent_instance;

  MnbClipboardViewPrivate *priv;
};

struct _MnbClipboardViewClass
{
  NbtkWidgetClass parent_class;
};

GType mnb_clipboard_view_get_type (void) G_GNUC_CONST;

NbtkWidget *mnb_clipboard_view_new (MnbClipboardStore *store);

MnbClipboardStore *mnb_clipboard_view_get_store (MnbClipboardView *view);

void mnb_clipboard_view_filter (MnbClipboardView *view,
                                const gchar      *filter);

G_END_DECLS

#endif /* __MNB_CLIPBOARD_VIEW_H__ */

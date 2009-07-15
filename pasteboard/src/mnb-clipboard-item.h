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

#ifndef __MNB_CLIPBOARD_ITEM_H__
#define __MNB_CLIPBOARD_ITEM_H__

#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_CLIPBOARD_ITEM                 (mnb_clipboard_item_get_type ())
#define MNB_CLIPBOARD_ITEM(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_CLIPBOARD_ITEM, MnbClipboardItem))
#define MNB_IS_CLIPBOARD_ITEM(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_CLIPBOARD_ITEM))
#define MNB_CLIPBOARD_ITEM_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_CLIPBOARD_ITEM, MnbClipboardItemClass))
#define MNB_IS_CLIPBOARD_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_CLIPBOARD_ITEM))
#define MNB_CLIPBOARD_ITEM_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_CLIPBOARD_ITEM, MnbClipboardItemClass))

typedef struct _MnbClipboardItem                MnbClipboardItem;
typedef struct _MnbClipboardItemClass           MnbClipboardItemClass;

struct _MnbClipboardItem
{
  NbtkTable parent_instance;

  ClutterActor *contents;

  ClutterActor *time_label;

  ClutterActor *remove_button;
  ClutterActor *action_button;

  gint64 serial;
};

struct _MnbClipboardItemClass
{
  NbtkTableClass parent_class;

  void (* remove_clicked) (MnbClipboardItem *item);
  void (* action_clicked) (MnbClipboardItem *item);
};

GType mnb_clipboard_item_get_type (void) G_GNUC_CONST;

G_CONST_RETURN gchar *mnb_clipboard_item_get_contents (MnbClipboardItem *item);
gint64                mnb_clipboard_item_get_serial   (MnbClipboardItem *item);

void                  mnb_clipboard_item_show_action  (MnbClipboardItem *item);
void                  mnb_clipboard_item_hide_action  (MnbClipboardItem *item);

G_END_DECLS

#endif /* __MNB_CLIPBOARD_ITEM_H__ */

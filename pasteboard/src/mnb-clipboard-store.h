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

#ifndef __MNB_CLIPBOARD_STORE_H__
#define __MNB_CLIPBOARD_STORE_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MNB_TYPE_CLIPBOARD_ITEM_TYPE            (mnb_clipboard_item_type_get_type ())

#define MNB_TYPE_CLIPBOARD_STORE                (mnb_clipboard_store_get_type ())
#define MNB_CLIPBOARD_STORE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_CLIPBOARD_STORE, MnbClipboardStore))
#define MNB_IS_CLIPBOARD_STORE(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_CLIPBOARD_STORE))
#define MNB_CLIPBOARD_STORE_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_CLIPBOARD_STORE, MnbClipboardStoreClass))
#define MNB_IS_CLIPBOARD_STORE_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_CLIPBOARD_STORE))
#define MNB_CLIPBOARD_STORE_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_CLIPBOARD_STORE, MnbClipboardStoreClass))

typedef struct _MnbClipboardStore               MnbClipboardStore;
typedef struct _MnbClipboardStorePrivate        MnbClipboardStorePrivate;
typedef struct _MnbClipboardStoreClass          MnbClipboardStoreClass;

typedef enum {
  MNB_CLIPBOARD_ITEM_INVALID = 0,
  MNB_CLIPBOARD_ITEM_TEXT,
  MNB_CLIPBOARD_ITEM_URIS,
  MNB_CLIPBOARD_ITEM_IMAGE
} MnbClipboardItemType;

struct _MnbClipboardStore
{
  ClutterListModel parent_instance;

  MnbClipboardStorePrivate *priv;
};

struct _MnbClipboardStoreClass
{
  ClutterListModelClass parent_class;

  void (* selection_changed) (MnbClipboardStore *store,
                              const gchar       *selection);

  void (* item_added)   (MnbClipboardStore    *store,
                         MnbClipboardItemType  item_type);
  void (* item_removed) (MnbClipboardStore    *store,
                         gint64                serial);
};

GType mnb_clipboard_item_type_get_type (void) G_GNUC_CONST;
GType mnb_clipboard_store_get_type (void) G_GNUC_CONST;

MnbClipboardStore *mnb_clipboard_store_new (void);

gchar * mnb_clipboard_store_get_last_text (MnbClipboardStore *store,
                                           gint64            *mtime,
                                           gint64            *serial);
gchar **mnb_clipboard_store_get_last_uris (MnbClipboardStore *store,
                                           gint64            *mtime,
                                           gint64            *serial);

void mnb_clipboard_store_remove (MnbClipboardStore *store,
                                 gint64             serial);

void mnb_clipboard_store_save_selection (MnbClipboardStore *store);

void mnb_clipboard_store_clear (MnbClipboardStore *store);

G_END_DECLS

#endif /* __MNB_CLIPBOARD_STORE_H__ */

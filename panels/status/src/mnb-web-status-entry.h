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

#ifndef __MNB_WEB_STATUS_ENTRY_H__
#define __MNB_WEB_STATUS_ENTRY_H__

#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_WEB_STATUS_ENTRY                   (mnb_web_status_entry_get_type ())
#define MNB_WEB_STATUS_ENTRY(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_WEB_STATUS_ENTRY, MnbWebStatusEntry))
#define MNB_IS_WEB_STATUS_ENTRY(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_WEB_STATUS_ENTRY))
#define MNB_WEB_STATUS_ENTRY_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_WEB_STATUS_ENTRY, MnbWebStatusEntryClass))
#define MNB_IS_WEB_STATUS_ENTRY_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_WEB_STATUS_ENTRY))
#define MNB_WEB_STATUS_ENTRY_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_WEB_STATUS_ENTRY, MnbWebStatusEntryClass))

typedef struct _MnbWebStatusEntry          MnbWebStatusEntry;
typedef struct _MnbWebStatusEntryPrivate   MnbWebStatusEntryPrivate;
typedef struct _MnbWebStatusEntryClass     MnbWebStatusEntryClass;

struct _MnbWebStatusEntry
{
  NbtkWidget parent_instance;

  MnbWebStatusEntryPrivate *priv;
};

struct _MnbWebStatusEntryClass
{
  NbtkWidgetClass parent_class;

  void (* status_changed) (MnbWebStatusEntry *entry,
                           const gchar    *new_status_text);
  void (* update_cancelled) (MnbWebStatusEntry *entry);
};

GType mnb_web_status_entry_get_type (void);

NbtkWidget *mnb_web_status_entry_new (const gchar *service_name);

void mnb_web_status_entry_show_button (MnbWebStatusEntry *entry,
                                       gboolean        show);

gboolean mnb_web_status_entry_get_is_active (MnbWebStatusEntry *entry);
void     mnb_web_status_entry_set_is_active (MnbWebStatusEntry *entry,
                                             gboolean        is_active);
gboolean mnb_web_status_entry_get_in_hover  (MnbWebStatusEntry *entry);
void     mnb_web_status_entry_set_in_hover  (MnbWebStatusEntry *entry,
                                             gboolean        in_hover);

void                  mnb_web_status_entry_set_status_text (MnbWebStatusEntry *entry,
                                                            const gchar       *status_text,
                                                            GTimeVal          *status_time);
G_CONST_RETURN gchar *mnb_web_status_entry_get_status_text (MnbWebStatusEntry *entry,
                                                            GTimeVal          *status_time);

G_END_DECLS

#endif /* __MNB_WEB_STATUS_ENTRY_H__ */

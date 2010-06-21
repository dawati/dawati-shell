
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
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

#ifndef MNB_ENTRY_H
#define MNB_ENTRY_H

#include <glib-object.h>
#include <meego-panel/mpl-entry.h>

G_BEGIN_DECLS

#define MNB_TYPE_ENTRY mnb_entry_get_type()

#define MNB_ENTRY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_ENTRY, MnbEntry))

#define MNB_ENTRY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_ENTRY, MnbEntryClass))

#define MNB_IS_ENTRY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_ENTRY))

#define MNB_IS_ENTRY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_ENTRY))

#define MNB_ENTRY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_ENTRY, MnbEntryClass))

typedef struct
{
  MplEntry parent;
} MnbEntry;

typedef struct
{
  MplEntryClass parent_class;
} MnbEntryClass;

GType mnb_entry_get_type (void);

ClutterActor *  mnb_entry_new                     (const char *label);

gboolean        mnb_entry_get_has_keyboard_focus  (MnbEntry   *self);
void            mnb_entry_set_has_keyboard_focus  (MnbEntry   *self,
                                                   gboolean    keyboard_focus);

G_END_DECLS

#endif /* MNB_ENTRY_H */


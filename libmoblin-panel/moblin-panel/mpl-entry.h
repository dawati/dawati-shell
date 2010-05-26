
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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

#ifndef __MPL_ENTRY_H__
#define __MPL_ENTRY_H__

#include <mx/mx.h>

G_BEGIN_DECLS

#define MPL_TYPE_ENTRY                   (mpl_entry_get_type ())
#define MPL_ENTRY(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_ENTRY, MplEntry))
#define MPL_IS_ENTRY(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_ENTRY))
#define MPL_ENTRY_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_ENTRY, MplEntryClass))
#define MPL_IS_ENTRY_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_ENTRY))
#define MPL_ENTRY_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_ENTRY, MplEntryClass))

typedef struct _MplEntry          MplEntry;
typedef struct _MplEntryPrivate   MplEntryPrivate;
typedef struct _MplEntryClass     MplEntryClass;

/**
 * MplEntry:
 *
 * Entry widget.
 */
struct _MplEntry
{
  /*<private>*/
  MxFrame parent_instance;

  MplEntryPrivate *priv;
};

/**
 * MplEntryClass:
 * @button_clicked: signal closure for the #MplEntry::button-clicked signal
 * @text_changed: signal closure for the #MplEntry::text-changed signal
 *
 * Class struct for #MplEntry.
 */
struct _MplEntryClass
{
  /*<private>*/
  MxFrameClass parent_class;

  /*<public>*/
  /* Signals. */
  void (* button_clicked) (MplEntry *self);
  void (* text_changed)   (MplEntry *self);
};

GType mpl_entry_get_type (void) G_GNUC_CONST;

MxWidget *  mpl_entry_new       (const gchar   *label);

const gchar * mpl_entry_get_label (MplEntry     *self);
void          mpl_entry_set_label (MplEntry     *self,
                                   const gchar  *label);

const gchar * mpl_entry_get_text  (MplEntry     *self);
void          mpl_entry_set_text  (MplEntry     *self,
                                   const gchar  *text);

MxWidget * mpl_entry_get_mx_entry  (MplEntry     *self);

G_END_DECLS

#endif /* __MPL_ENTRY_H__ */

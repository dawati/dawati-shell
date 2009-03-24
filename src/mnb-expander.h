/*
 * Copyright (C) 2009 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Robert Staudinger <robsta@openedhand.com>
 */

#ifndef __MNB_EXPANDER_H__
#define __MNB_EXPANDER_H__

#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_EXPANDER                (mnb_expander_get_type ())
#define MNB_EXPANDER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_EXPANDER, MnbExpander))
#define MNB_IS_EXPANDER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_EXPANDER))
#define MNB_EXPANDER_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_EXPANDER, MnbExpanderClass))
#define MNB_IS_EXPANDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_EXPANDER))
#define MNB_EXPANDER_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_EXPANDER, MnbExpanderClass))

typedef struct _MnbExpander              MnbExpander;
typedef struct _MnbExpanderPrivate       MnbExpanderPrivate;
typedef struct _MnbExpanderClass         MnbExpanderClass;

struct _MnbExpander
{
  /*< private >*/
  NbtkBin parent_instance;

  MnbExpanderPrivate *priv;
};

struct _MnbExpanderClass
{
  NbtkBinClass parent_class;
};

GType mnb_expander_get_type (void) G_GNUC_CONST;

NbtkWidget *    mnb_expander_new       (const gchar *label);

gboolean        mnb_expander_get_expanded  (MnbExpander *self);
void            mnb_expander_set_expanded  (MnbExpander *self,
                                             gboolean      expanded);

ClutterActor *  mnb_expander_get_child     (MnbExpander *self);
const gchar *   mnb_expander_get_label     (MnbExpander *self);

G_END_DECLS

#endif /* __MNB_EXPANDER_H__ */


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

#ifndef MNB_FILTER_H
#define MNB_FILTER_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MPD_TYPE_FILTER mnb_filter_get_type()

#define MNB_FILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_FILTER, MnbFilter))

#define MNB_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_FILTER, MnbFilterClass))

#define MPD_IS_FILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_FILTER))

#define MPD_IS_FILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_FILTER))

#define MNB_FILTER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_FILTER, MnbFilterClass))

typedef struct
{
  MxBoxLayout parent;
} MnbFilter;

typedef struct
{
  MxBoxLayoutClass parent;
} MnbFilterClass;

GType
mnb_filter_get_type (void);

ClutterActor *
mnb_filter_new (void);

char const *
mnb_filter_get_text (MnbFilter *self);

void
mnb_filter_set_text (MnbFilter  *self,
                     char const *text);

G_END_DECLS

#endif /* MNB_FILTER_H */



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

#ifndef MNB_EXPANDER_H
#define MNB_EXPANDER_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MNB_TYPE_EXPANDER mnb_expander_get_type()

#define MNB_EXPANDER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_EXPANDER, MnbExpander))

#define MNB_EXPANDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_EXPANDER, MnbExpanderClass))

#define MNB_IS_EXPANDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_EXPANDER))

#define MNB_IS_EXPANDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_EXPANDER))

#define MNB_EXPANDER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_EXPANDER, MnbExpanderClass))

typedef struct
{
  MxExpander parent;
} MnbExpander;

typedef struct
{
  MxExpanderClass parent_class;
} MnbExpanderClass;

GType mnb_expander_get_type (void);

MnbExpander* mnb_expander_new (void);

G_END_DECLS

#endif /* MNB_EXPANDER_H */


/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2010, Intel Corporation.
 *
 * Authors: Danielle Madeley <danielle.madeley@collabora.co.uk>
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
 *
 */

#ifndef __ANERLEY_PRESENCE_CHOOSER_H__
#define __ANERLEY_PRESENCE_CHOOSER_H__

#include <mx/mx.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_PRESENCE_CHOOSER	(anerley_presence_chooser_get_type ())
#define ANERLEY_PRESENCE_CHOOSER(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_PRESENCE_CHOOSER, AnerleyPresenceChooser))
#define ANERLEY_PRESENCE_CHOOSER_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), ANERLEY_TYPE_PRESENCE_CHOOSER, AnerleyPresenceChooserClass))
#define ANERLEY_IS_PRESENCE_CHOOSER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_PRESENCE_CHOOSER))
#define ANERLEY_IS_PRESENCE_CHOOSER_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), ANERLEY_TYPE_PRESENCE_CHOOSER))
#define ANERLEY_PRESENCE_CHOOSER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_PRESENCE_CHOOSER, AnerleyPresenceChooserClass))

typedef struct _AnerleyPresenceChooser AnerleyPresenceChooser;
typedef struct _AnerleyPresenceChooserClass AnerleyPresenceChooserClass;

struct _AnerleyPresenceChooser
{
  MxComboBox parent;
};

struct _AnerleyPresenceChooserClass
{
  MxComboBoxClass parent_class;
};

GType anerley_presence_chooser_get_type (void);
ClutterActor *anerley_presence_chooser_new (void);

G_END_DECLS

#endif

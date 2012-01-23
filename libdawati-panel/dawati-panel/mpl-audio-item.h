/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mpl-audio-item.h */
/*
 * Copyright (c) 2011 Intel Corp.
 *
 * Author: Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _MPL_AUDIO_ITEM
#define _MPL_AUDIO_ITEM

#include <clutter/clutter.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MPL_TYPE_AUDIO_ITEM mpl_audio_item_get_type()

#define MPL_AUDIO_ITEM(obj)                                             \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_AUDIO_ITEM, MplAudioItem))

#define MPL_AUDIO_ITEM_CLASS(klass)                                     \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_AUDIO_ITEM, MplAudioItemClass))

#define MPL_IS_AUDIO_ITEM(obj)                                  \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_AUDIO_ITEM))

#define MPL_IS_AUDIO_ITEM_CLASS(klass)                          \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_AUDIO_ITEM))

#define MPL_AUDIO_ITEM_GET_CLASS(obj)                                   \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_AUDIO_ITEM, MplAudioItemClass))

typedef struct _MplAudioItem        MplAudioItem;
typedef struct _MplAudioItemClass   MplAudioItemClass;
typedef struct _MplAudioItemPrivate MplAudioItemPrivate;

struct _MplAudioItem
{
  /*< private >*/
  MxBin parent_instance;

  MplAudioItemPrivate *priv;
};

struct _MplAudioItemClass
{
  MxBinClass parent_class;
};

GType mpl_audio_item_get_type (void) G_GNUC_CONST;

ClutterActor *mpl_audio_item_new (void);

G_END_DECLS

#endif

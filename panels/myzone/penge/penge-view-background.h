/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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


#ifndef _PENGE_VIEW_BACKGROUND
#define _PENGE_VIEW_BACKGROUND

#include <glib-object.h>
#include "penge-magic-texture.h"

G_BEGIN_DECLS

#define PENGE_TYPE_VIEW_BACKGROUND penge_view_background_get_type()

#define PENGE_VIEW_BACKGROUND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_VIEW_BACKGROUND, PengeViewBackground))

#define PENGE_VIEW_BACKGROUND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_VIEW_BACKGROUND, PengeViewBackgroundClass))

#define PENGE_IS_VIEW_BACKGROUND(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_VIEW_BACKGROUND))

#define PENGE_IS_VIEW_BACKGROUND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_VIEW_BACKGROUND))

#define PENGE_VIEW_BACKGROUND_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_VIEW_BACKGROUND, PengeViewBackgroundClass))

typedef struct _PengeViewBackgroundPrivate PengeViewBackgroundPrivate;

typedef struct {
  PengeMagicTexture parent;
  PengeViewBackgroundPrivate *priv;
} PengeViewBackground;

typedef struct {
  PengeMagicTextureClass parent_class;
} PengeViewBackgroundClass;

GType penge_view_background_get_type (void);

G_END_DECLS

#endif /* _PENGE_VIEW_BACKGROUND */


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


#ifndef _PENGE_MAGIC_TEXTURE
#define _PENGE_MAGIC_TEXTURE

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define PENGE_TYPE_MAGIC_TEXTURE penge_magic_texture_get_type()

#define PENGE_MAGIC_TEXTURE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_MAGIC_TEXTURE, PengeMagicTexture))

#define PENGE_MAGIC_TEXTURE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_MAGIC_TEXTURE, PengeMagicTextureClass))

#define PENGE_IS_MAGIC_TEXTURE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_MAGIC_TEXTURE))

#define PENGE_IS_MAGIC_TEXTURE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_MAGIC_TEXTURE))

#define PENGE_MAGIC_TEXTURE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_MAGIC_TEXTURE, PengeMagicTextureClass))

typedef struct {
  ClutterTexture parent;
} PengeMagicTexture;

typedef struct {
  ClutterTextureClass parent_class;
} PengeMagicTextureClass;

GType penge_magic_texture_get_type (void);

G_END_DECLS

#endif /* _PENGE_MAGIC_TEXTURE */


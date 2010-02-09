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

#ifndef __ANERLEY_TP_USER_AVATAR_H__
#define __ANERLEY_TP_USER_AVATAR_H__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_TP_USER_AVATAR	(anerley_tp_user_avatar_get_type ())
#define ANERLEY_TP_USER_AVATAR(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_TP_USER_AVATAR, AnerleyTpUserAvatar))
#define ANERLEY_TP_USER_AVATAR_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), ANERLEY_TYPE_TP_USER_AVATAR, AnerleyTpUserAvatarClass))
#define ANERLEY_IS_TP_USER_AVATAR(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_TP_USER_AVATAR))
#define ANERLEY_IS_TP_USER_AVATAR_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), ANERLEY_TYPE_TP_USER_AVATAR))
#define ANERLEY_TP_USER_AVATAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_TP_USER_AVATAR, AnerleyTpUserAvatarClass))

typedef struct _AnerleyTpUserAvatar AnerleyTpUserAvatar;
typedef struct _AnerleyTpUserAvatarClass AnerleyTpUserAvatarClass;

struct _AnerleyTpUserAvatar
{
  ClutterTexture parent;
};

struct _AnerleyTpUserAvatarClass
{
  ClutterTextureClass parent_class;
};

GType anerley_tp_user_avatar_get_type (void);
ClutterActor *anerley_tp_user_avatar_new (void);

G_END_DECLS

#endif

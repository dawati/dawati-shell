/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Tomas Frydrych <tf@linux.intel.com>
 *
 * Copyright (C) 2008 Intel Corporation
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
 */

#ifndef __NUTTER_WS_ICON_H__
#define __NUTTER_WS_ICON_H__

#include <clutter/clutter-actor.h>

G_BEGIN_DECLS

#define NUTTER_TYPE_WS_ICON               (nutter_ws_icon_get_type())
#define NUTTER_WS_ICON(obj)                                       \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),				  \
                               NUTTER_TYPE_WS_ICON,               \
                               NutterWsIcon))
#define NUTTER_WS_ICON_CLASS(klass)                               \
  (G_TYPE_CHECK_CLASS_CAST ((klass),				  \
                            NUTTER_TYPE_WS_ICON,                  \
                            NutterWsIconClass))
#define NUTTER_IS_WS_ICON(obj)                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),				  \
                               NUTTER_TYPE_WS_ICON))
#define NUTTER_IS_WS_ICON_CLASS(klass)			     \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                         \
                            NUTTER_TYPE_WS_ICON))
#define NUTTER_WS_ICON_GET_CLASS(obj)                             \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),				  \
                              NUTTER_TYPE_WS_ICON,                \
                              NutterWsIconClass))

typedef struct _NutterWsIcon        NutterWsIcon;
typedef struct _NutterWsIconClass   NutterWsIconClass;
typedef struct _NutterWsIconPrivate NutterWsIconPrivate;

struct _NutterWsIconClass
{
  ClutterActorClass parent_class;
};

struct _NutterWsIcon
{
  ClutterActor parent;

  NutterWsIconPrivate *priv;
};

GType nutter_ws_icon_get_type (void) G_GNUC_CONST;

ClutterActor *nutter_ws_icon_new                    (void);

void          nutter_ws_set_text             (NutterWsIcon *self,
					      const gchar  *text);

void          nutter_ws_icon_get_color        (NutterWsIcon   *rectangle,
					       ClutterColor       *color);
void          nutter_ws_icon_set_color        (NutterWsIcon   *rectangle,
					       const ClutterColor *color);
guint         nutter_ws_icon_get_border_width (NutterWsIcon   *rectangle);
void          nutter_ws_icon_set_border_width (NutterWsIcon   *rectangle,
					       guint               width);
void          nutter_ws_icon_get_border_color (NutterWsIcon   *rectangle,
					       ClutterColor       *color);
void          nutter_ws_icon_set_border_color (NutterWsIcon   *rectangle,
					       const ClutterColor *color);

void          nutter_ws_icon_get_text_color (NutterWsIcon *self,
					     ClutterColor *color);

void          nutter_ws_icon_set_text_color (NutterWsIcon       *self,
					     const ClutterColor *color);

const gchar * nutter_ws_icon_get_font_name (NutterWsIcon *self);

void          nutter_ws_icon_set_font_name (NutterWsIcon *self,
					    const gchar  *font);

const gchar *nutter_ws_icon_get_text (NutterWsIcon *self);

void         nutter_ws_icon_set_text (NutterWsIcon *self, const gchar *text);

G_END_DECLS

#endif /* __NUTTER_WS_ICON_H__ */

/*
 * Copyright © 2009, 2010, Intel Corporation.
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
 *
 * Authors: Thomas Wood <thomas.wood@intel.com>
 */

#ifndef _SW_WINDOW_H
#define _SW_WINDOW_H

#include <mx/mx.h>

G_BEGIN_DECLS

#define SW_TYPE_WINDOW sw_window_get_type()

#define SW_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  SW_TYPE_WINDOW, SwWindow))

#define SW_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  SW_TYPE_WINDOW, SwWindowClass))

#define SW_IS_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  SW_TYPE_WINDOW))

#define SW_IS_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  SW_TYPE_WINDOW))

#define SW_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  SW_TYPE_WINDOW, SwWindowClass))

typedef struct _SwWindow SwWindow;
typedef struct _SwWindowClass SwWindowClass;
typedef struct _SwWindowPrivate SwWindowPrivate;

struct _SwWindow
{
  MxWidget parent;

  SwWindowPrivate *priv;
};

struct _SwWindowClass
{
  MxWidgetClass parent_class;
};

GType sw_window_get_type (void) G_GNUC_CONST;

ClutterActor *sw_window_new (void);

void   sw_window_set_xid     (SwWindow *window, gulong xid);
gulong sw_window_get_xid     (SwWindow *window);

void   sw_window_set_focused (SwWindow *window, gboolean        focused);
void   sw_window_set_icon    (SwWindow *window, ClutterTexture *icon);
void   sw_window_set_title   (SwWindow *window, const gchar    *title);

void   sw_window_set_thumbnail (SwWindow *window, ClutterActor *thumbnail);
void   sw_window_set_background (SwWindow *window, ClutterActor *thumbnail);

void   sw_window_workspace_changed (SwWindow *window, gint new_workspace);

G_END_DECLS

#endif /* _SW_WINDOW_H */

/*
 * Moblin-Web-Browser: The web browser for Moblin
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef _MWB_UTILS_H
#define _MWB_UTILS_H

#include <glib-object.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define MWB_PIXBOUND(u) ((gfloat)((gint)(u)))

/* We want to ignore the values of the following modifiers when
   checking for keyboard shortcuts */
#define MWB_UTILS_MODIFIERS_MASK                   \
  (~(CLUTTER_LOCK_MASK                             \
     | CLUTTER_MOD2_MASK                           \
     | CLUTTER_MOD3_MASK                           \
     | CLUTTER_MOD4_MASK                           \
     | CLUTTER_MOD5_MASK                           \
     | CLUTTER_BUTTON1_MASK                        \
     | CLUTTER_BUTTON2_MASK                        \
     | CLUTTER_BUTTON3_MASK                        \
     | CLUTTER_BUTTON4_MASK                        \
     | CLUTTER_BUTTON5_MASK))


gboolean
mwb_utils_focus_on_click_cb (ClutterActor       *actor,
                             ClutterButtonEvent *event,
                             gpointer            swallow_event);

CoglHandle
mwb_utils_image_to_texture (const guint8 *data,
                            guint data_len,
                            GError **error);

inline void
mwb_utils_table_add (NbtkTable    *table,
                     ClutterActor *child,
                     gint          row,
                     gint          col,
                     gboolean      x_expand,
                     gboolean      x_fill,
                     gboolean      y_expand,
                     gboolean      y_fill);

gboolean
mwb_utils_actor_has_focus (ClutterActor *actor);

void
mwb_utils_show_popup (NbtkPopup *popup);

GdkPixbuf *
mwb_utils_pixbuf_new_from_stock (const gchar *icon_name, gint size);

GdkCursor *
mwb_utils_cursor_new_from_stock (const gchar *icon_name);

G_END_DECLS

#endif /* _MWB_UTILS_H */


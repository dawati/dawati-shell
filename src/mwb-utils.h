/* mwb-utils.h */
/*
 * Copyright (c) 2009 Intel Corp.
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

/* Borrowed from the moblin-web-browser project */

#ifndef _MWB_UTILS_H
#define _MWB_UTILS_H

#include <glib-object.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

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

G_END_DECLS

#endif /* _MWB_UTILS_H */

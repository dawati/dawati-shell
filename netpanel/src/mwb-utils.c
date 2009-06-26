/* mwb-utils.c */
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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "mwb-utils.h"

gboolean
mwb_utils_focus_on_click_cb (ClutterActor       *actor,
                             ClutterButtonEvent *event,
                             gpointer            swallow_event)
{
  ClutterActor *stage = clutter_actor_get_stage (actor);

  if (stage && CLUTTER_IS_STAGE (stage))
    clutter_stage_set_key_focus (CLUTTER_STAGE (stage), actor);

  return GPOINTER_TO_INT (swallow_event);
}

CoglHandle
mwb_utils_image_to_texture (const guint8 *data,
                            guint data_len,
                            GError **error)
{
  GdkPixbuf *pixbuf;
  GInputStream *input_stream
    = g_memory_input_stream_new_from_data (data, data_len, NULL);
  ClutterActor *texture = COGL_INVALID_HANDLE;

  if ((pixbuf = gdk_pixbuf_new_from_stream_at_scale (input_stream,
                                                     16, 16,
                                                     TRUE,
                                                     NULL,
                                                     error)))
    {
      texture = cogl_texture_new_from_data (gdk_pixbuf_get_width (pixbuf),
                                            gdk_pixbuf_get_height (pixbuf),
                                            0,
                                            gdk_pixbuf_get_has_alpha (pixbuf)
                                            ? COGL_PIXEL_FORMAT_RGBA_8888
                                            : COGL_PIXEL_FORMAT_RGB_888,
                                            COGL_PIXEL_FORMAT_ANY,
                                            gdk_pixbuf_get_rowstride (pixbuf),
                                            gdk_pixbuf_get_pixels (pixbuf));

      g_object_unref (pixbuf);
    }

  g_object_unref (input_stream);

  return texture;
}

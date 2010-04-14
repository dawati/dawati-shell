
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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

#include <stdbool.h>
#include <clutter-gtk/clutter-gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "mpd-battery-icon.h"

G_DEFINE_TYPE (MpdBatteryIcon, mpd_battery_icon, CLUTTER_TYPE_TEXTURE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_BATTERY_ICON, MpdBatteryIconPrivate))

enum {
  PROP_0,

  PROP_FPS
};

typedef struct
{
  unsigned int fps;

  /* Only while animating. */
  GList const *iter;
} MpdBatteryIconPrivate;

static void
_get_property (GObject      *object,
               unsigned int  property_id,
               GValue       *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_FPS:
    g_value_set_uint (value,
                      mpd_battery_icon_get_fps (MPD_BATTERY_ICON (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               unsigned int  property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  switch (property_id) {
  case PROP_FPS:
    mpd_battery_icon_set_fps (MPD_BATTERY_ICON (object),
                              g_value_get_uint (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_battery_icon_parent_class)->dispose (object);
}

static void
mpd_battery_icon_class_init (MpdBatteryIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdBatteryIconPrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;

  /* Properties */

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_FPS,
                                   g_param_spec_uint ("fps",
                                                      "Frames per second",
                                                      "Animation speed",
                                                      1, 60, 30,
                                                      param_flags));
}

static void
mpd_battery_icon_init (MpdBatteryIcon *self)
{
}

MpdBatteryIcon *
mpd_battery_icon_new (void)
{
  return g_object_new (MPD_TYPE_BATTERY_ICON, NULL);
}

unsigned int
mpd_battery_icon_get_fps (MpdBatteryIcon *self)
{
  MpdBatteryIconPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_BATTERY_ICON (self), 0);

  return priv->fps;
}

void
mpd_battery_icon_set_fps (MpdBatteryIcon *self,
                          unsigned int    fps)
{
  MpdBatteryIconPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_BATTERY_ICON (self));
  g_return_if_fail (fps > 0);
  g_return_if_fail (fps <= 60);

  if (fps != priv->fps)
  {
    priv->fps = fps;
    g_object_notify (G_OBJECT (self), "fps");
  }
}

static bool
render_frame (MpdBatteryIcon *self)
{
  MpdBatteryIconPrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;

  /* At last frame? */
  if (NULL == priv->iter)
    return false;

  gtk_clutter_texture_set_from_pixbuf (CLUTTER_TEXTURE (self),
                                       GDK_PIXBUF (priv->iter->data),
                                       &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
    priv->iter = NULL;
    return false;
  }

  priv->iter = priv->iter->next;
  return true;
}

static bool
_next_frame_cb (MpdBatteryIcon  *self)
{
  return render_frame (self);
}

void
mpd_battery_icon_animate (MpdBatteryIcon  *self,
                          GList           *frames)
{
  MpdBatteryIconPrivate *priv = GET_PRIVATE (self);

  priv->iter = frames;
  render_frame (self);
  g_timeout_add (priv->fps * 1000, (GSourceFunc) _next_frame_cb, self);
}

GList *
mpd_battery_icon_load_frames_from_dir (char const  *path,
                                       GError     **error)
{
  GDir        *dir;
  char const  *entry;
  GList       *frames = NULL;

  dir = g_dir_open (path, 0, error);
  if (NULL == dir)
  {
    return NULL;
  }

  while (NULL != (entry = g_dir_read_name (dir)))
  {
    if (entry[0] != '.')
    {
      char *filename = g_build_filename (path, entry, NULL);
      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (filename, error);
      g_free (filename);
      if (NULL == pixbuf)
      {
        return frames;
      }
      /* List takes reference. */
      frames = g_list_prepend (frames, pixbuf);
    }
  }

  frames = g_list_reverse (frames);
  frames = g_list_sort (frames, (GCompareFunc) g_strcmp0);

  return frames;
}


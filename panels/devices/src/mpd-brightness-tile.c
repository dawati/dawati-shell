
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

#include <glib/gi18n.h>

#include "gpm-brightness-xrandr.h"
#include "mpd-gobject.h"
#include "mpd-brightness-tile.h"

G_DEFINE_TYPE (MpdBrightnessTile, mpd_brightness_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_BRIGHTNESS_TILE, MpdBrightnessTilePrivate))

typedef struct
{
  MxSlider            *slider;
  GpmBrightnessXRandR *brightness;
} MpdBrightnessTilePrivate;

static void
update_display_brightness (MpdBrightnessTile *self);

static void
update_brightness_slider (MpdBrightnessTile *self);

static void
_brightness_slider_value_notify_cb (MxSlider          *slider,
                                    GParamSpec        *pspec,
                                    MpdBrightnessTile *self)
{
  update_display_brightness (self);
}

static void
_brightness_changed_cb (GpmBrightnessXRandR *brightness,
                        unsigned int         percentage,
                        MpdBrightnessTile   *self)
{
  update_brightness_slider (self);
}

static void
_dispose (GObject *object)
{
  MpdBrightnessTilePrivate *priv = GET_PRIVATE (object);

  mpd_gobject_detach (object, (GObject **) &priv->brightness);

  G_OBJECT_CLASS (mpd_brightness_tile_parent_class)->dispose (object);
}

static void
mpd_brightness_tile_class_init (MpdBrightnessTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdBrightnessTilePrivate));

  object_class->dispose = _dispose;
}

static void
mpd_brightness_tile_init (MpdBrightnessTile *self)
{
  MpdBrightnessTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *label;

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);

  label = mx_label_new_with_text (_("Netbook brightness"));
  clutter_actor_set_name (label, "brightness-tile-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), label);

  priv->slider = (MxSlider *) mx_slider_new ();
  g_signal_connect (priv->slider, "notify::value",
                    G_CALLBACK (_brightness_slider_value_notify_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->slider));
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->slider),
                                "expand", true,
                                "x-fill", true,
                                "y-fill", false,
                                NULL);

  priv->brightness = gpm_brightness_xrandr_new ();
  g_signal_connect (priv->brightness, "brightness-changed",
                    G_CALLBACK (_brightness_changed_cb), self);

  update_brightness_slider (self);
}

ClutterActor *
mpd_brightness_tile_new (void)
{
  return g_object_new (MPD_TYPE_BRIGHTNESS_TILE, NULL);
}

static void
update_display_brightness (MpdBrightnessTile *self)
{
  MpdBrightnessTilePrivate *priv = GET_PRIVATE (self);
  double    value;
  gboolean  hw_changed;
  bool      ret;

  g_signal_handlers_disconnect_by_func (priv->brightness,
                                        _brightness_changed_cb,
                                        self);

  value = mx_slider_get_value (priv->slider);
  ret = gpm_brightness_xrandr_set (priv->brightness, value * 100, &hw_changed);
  if (!ret)
  {
    g_warning ("%s : Setting brightness failed", G_STRLOC);
  }

  g_signal_connect (priv->brightness, "brightness-changed",
                    G_CALLBACK (_brightness_changed_cb), self);

}

static void
update_brightness_slider (MpdBrightnessTile *self)
{
  MpdBrightnessTilePrivate *priv = GET_PRIVATE (self);
  unsigned int  percentage;
  bool          ret;

  g_signal_handlers_disconnect_by_func (priv->slider,
                                        _brightness_slider_value_notify_cb,
                                        self);

  ret = gpm_brightness_xrandr_get (priv->brightness, &percentage);
  if (ret)
  {
    mx_slider_set_value (priv->slider, percentage / (double) 100);
  } else {
    g_warning ("%s : Getting brightness failed", G_STRLOC);
  }

  g_signal_connect (priv->slider, "notify::value",
                    G_CALLBACK (_brightness_slider_value_notify_cb), self);
}


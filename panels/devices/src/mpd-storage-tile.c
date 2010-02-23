
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

#include <glib/gi18n.h>
#include "mpd-gobject.h"
#include "mpd-storage-tile.h"

static void
mpd_storage_tile_set_device (MpdStorageTile *self,
                             GduDevice      *device);

G_DEFINE_TYPE (MpdStorageTile, mpd_storage_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_STORAGE_TILE, MpdStorageTilePrivate))

enum
{
  PROP_0,

  PROP_DEVICE
};

typedef struct
{
  /* Managed by clutter */
  ClutterActor *icon;
  ClutterActor *title;
  ClutterActor *description;
  ClutterActor *meter;

  GduDevice     *device;
} MpdStorageTilePrivate;

static void
_eject_clicked_cb (MxButton       *button,
                   MpdStorageTile *self)
{

}

static void
_open_clicked_cb (MxButton       *button,
                  MpdStorageTile *self)
{

}

static GObject *
_constructor (GType                  type,
              unsigned int           n_properties,
              GObjectConstructParam *properties)
{
  MpdStorageTile *self = (MpdStorageTile *)
                            G_OBJECT_CLASS (mpd_storage_tile_parent_class)
                              ->constructor (type, n_properties, properties);
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);

  if (priv->device == NULL)
  {
    g_critical ("%s : %s",
                G_STRLOC,
                "Invalid or no device passed to constructor.");
    g_object_unref (self);
    self = NULL;
  }

  return (GObject *) self;
}

static void
_get_property (GObject      *object,
               unsigned int  property_id,
               GValue       *value, 
               GParamSpec   *pspec)
{
  switch (property_id) {
  case PROP_DEVICE:
    g_value_set_object (value,
                        mpd_storage_tile_get_device (MPD_STORAGE_TILE (object)));
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
  case PROP_DEVICE:
    mpd_storage_tile_set_device (MPD_STORAGE_TILE (object),
                                 g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (object);

  mpd_gobject_detach (object, (GObject **) &priv->device);

  G_OBJECT_CLASS (mpd_storage_tile_parent_class)->dispose (object);
}

static void
mpd_storage_tile_class_init (MpdStorageTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdStorageTilePrivate));

  object_class->constructor = _constructor;
  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;

  /* Properties */

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_DEVICE,
                                   g_param_spec_object ("device",
                                                        "Device",
                                                        "The device object",
                                                        GDU_TYPE_DEVICE,
                                                        param_flags | 
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
mpd_storage_tile_init (MpdStorageTile *self)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor  *vbox;
  ClutterActor  *button;

  /* 1st column: icon */
  priv->icon = clutter_texture_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->icon);

  /* 2nd column: text, free space */
  vbox = mx_box_layout_new ();
  mx_box_layout_set_vertical (MX_BOX_LAYOUT (vbox), TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), vbox);

  priv->title = mx_label_new ("");
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), priv->title);

  priv->description = mx_label_new ("");
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), priv->description);

  priv->meter = mx_progress_bar_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), priv->meter);

  /* 3rd column: buttons */
  vbox = mx_box_layout_new ();
  mx_box_layout_set_vertical (MX_BOX_LAYOUT (vbox), TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), vbox);

  button = mx_button_new_with_label (_("Eject"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_eject_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), button);

  button = mx_button_new_with_label (_("Open"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_open_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), button);
}

MpdStorageTile *
mpd_storage_tile_new (GduDevice *device)
{
  return g_object_new (MPD_TYPE_STORAGE_TILE,
                       "device", device,
                       NULL);
}

GduDevice *
mpd_storage_tile_get_device (MpdStorageTile *self)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_TILE (self), NULL);

  return priv->device;
}

static void
mpd_storage_tile_set_device (MpdStorageTile *self,
                             GduDevice      *device)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_STORAGE_TILE (self));

  if (device != priv->device)
  {
    if (priv->device)
      mpd_gobject_detach (G_OBJECT (self), (GObject **) &priv->device);

    if (device)
    {
      GError *error = NULL;

      priv->device = g_object_ref (device);

      /* Icon */
      clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon),
                                     PKGICONDIR "/device-usb.png",
                                     &error);
      if (error)
      {
        g_warning ("%s : %s", G_STRLOC, error->message);
        g_clear_error (&error);
      }

      /* Title */
      mx_label_set_text (MX_LABEL (priv->title),
                         gdu_device_get_presentation_name (priv->device));

      // todo
    }

    g_object_notify (G_OBJECT (self), "device");
  }
}


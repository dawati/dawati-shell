
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

#include "mpd-gobject.h"
#include "mpd-shell-defines.h"
#include "mpd-storage-device.h"
#include "mpd-storage-tile.h"
#include "config.h"

static void
mpd_storage_tile_set_mount_point (MpdStorageTile *self,
                                  char const     *mount_point);

static void
mpd_storage_tile_set_title (MpdStorageTile *self,
                            char const     *title);

G_DEFINE_TYPE (MpdStorageTile, mpd_storage_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_STORAGE_TILE, MpdStorageTilePrivate))

enum
{
  PROP_0,

  PROP_MOUNT_POINT,
  PROP_TITLE
};

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

typedef struct
{
  /* Managed by clutter */
  ClutterActor      *icon;
  ClutterActor      *title;
  ClutterActor      *description;
  ClutterActor      *meter;

  char              *mount_point;
  MpdStorageDevice  *storage;
} MpdStorageTilePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
update (MpdStorageTile *self)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);
  char          *text;
  char          *size_text;
  uint64_t       size;
  uint64_t       available_size;
  unsigned int   percentage;

  g_object_get (priv->storage,
                "size", &size,
                "available-size", &available_size,
                NULL);

  percentage = 100 - (double) available_size / size * 100;

  size_text = g_format_size_for_display (size);
  text = g_strdup_printf (_("Using %d%% of %s"), percentage, size_text);
  mx_label_set_text (MX_LABEL (priv->description), text);
  g_free (text);
  g_free (size_text);

  mx_progress_bar_set_progress (MX_PROGRESS_BAR (priv->meter), percentage / 100.);
}

static void
_storage_size_notify_cb (MpdStorageDevice  *storage,
                         GParamSpec        *pspec,
                         MpdStorageTile    *self)
{
  update (self);
}

static void
_eject_clicked_cb (MxButton       *button,
                   MpdStorageTile *self)
{
  // TODO
}

static void
_open_clicked_cb (MxButton       *button,
                  MpdStorageTile *self)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);
  char    *command_line;
  GError  *error = NULL;

  command_line = g_strdup_printf ("nautilus file://%s", priv->mount_point);
  g_spawn_command_line_async (command_line, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
  g_free (command_line);

  g_signal_emit_by_name (self, "request-hide");
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

  if (priv->mount_point == NULL)
  {
    g_critical ("%s : %s",
                G_STRLOC,
                "Invalid or no mount-point passed to constructor.");
    g_object_unref (self);
    self = NULL;
  }

  priv->storage = mpd_storage_device_new (priv->mount_point);
  g_signal_connect (priv->storage, "notify::size",
                    G_CALLBACK (_storage_size_notify_cb), self);
  g_signal_connect (priv->storage, "notify::available-size",
                    G_CALLBACK (_storage_size_notify_cb), self);
  update (self);

  return (GObject *) self;
}

static void
_get_property (GObject      *object,
               unsigned int  property_id,
               GValue       *value, 
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_MOUNT_POINT:
    g_value_set_string (value,
                        mpd_storage_tile_get_mount_point (
                          MPD_STORAGE_TILE (object)));
    break;
  case PROP_TITLE:
    g_value_set_string (value,
                        mpd_storage_tile_get_title (MPD_STORAGE_TILE (object)));
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
  switch (property_id)
  {
  case PROP_MOUNT_POINT:
    mpd_storage_tile_set_mount_point (MPD_STORAGE_TILE (object),
                                     g_value_get_string (value));
    break;
  case PROP_TITLE:
    mpd_storage_tile_set_title (MPD_STORAGE_TILE (object),
                                g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (object);

  if (priv->mount_point)
  {
    g_free (priv->mount_point);
    priv->mount_point = NULL;
  }

  mpd_gobject_detach (object, (GObject **) &priv->storage);

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
                                   PROP_MOUNT_POINT,
                                   g_param_spec_string ("mount-point",
                                                        "Mount point",
                                                        "Device mount point",
                                                        NULL,
                                                        param_flags | 
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "Storage device title",
                                                        NULL,
                                                        param_flags | 
                                                        G_PARAM_CONSTRUCT_ONLY));

  /* Signals */

  _signals[REQUEST_HIDE] = g_signal_new ("request-hide",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);
}

static void
mpd_storage_tile_init (MpdStorageTile *self)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor  *vbox;
  ClutterActor  *button;
  GError        *error = NULL;

  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), MPD_STORAGE_TILE_SPACING);

  /* 1st column: icon */
  priv->icon = clutter_texture_new ();
  clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon),
                                 PKGICONDIR "/device-usb.png",
                                 &error);
  clutter_texture_set_sync_size (CLUTTER_TEXTURE (priv->icon), true);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->icon);
  clutter_container_child_set (CLUTTER_CONTAINER (self), priv->icon,
                               "expand", false,
                               "x-align", MX_ALIGN_START,
                               "x-fill", false,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", false,
                               NULL);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  /* 2nd column: text, free space */
  vbox = mx_box_layout_new ();
  mx_box_layout_set_vertical (MX_BOX_LAYOUT (vbox), true);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), vbox);
  clutter_container_child_set (CLUTTER_CONTAINER (self), vbox,
                               "expand", true,
                               "x-align", MX_ALIGN_START,
                               "x-fill", true,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", false,
                               NULL);

  priv->title = mx_label_new ("");
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), priv->title);

  priv->description = mx_label_new ("");
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), priv->description);

  priv->meter = mx_progress_bar_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), priv->meter);

  /* 3rd column: buttons */
  vbox = mx_box_layout_new ();
  mx_box_layout_set_vertical (MX_BOX_LAYOUT (vbox), true);
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

ClutterActor *
mpd_storage_tile_new (char const *mount_point,
                      char const *title)
{
  return g_object_new (MPD_TYPE_STORAGE_TILE,
                       "mount-point", mount_point,
                       "title", title,
                       NULL);
}

char const *
mpd_storage_tile_get_mount_point (MpdStorageTile *self)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_TILE (self), NULL);

  return priv->mount_point;
}

static void
mpd_storage_tile_set_mount_point (MpdStorageTile *self,
                                  char const     *mount_point)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_STORAGE_TILE (self));

  if (0 != g_strcmp0 (mount_point, priv->mount_point))
  {
    if (priv->mount_point)
    {
      g_free (priv->mount_point);
      priv->mount_point = NULL;
    }

    if (mount_point)
    {
      priv->mount_point = g_strdup (mount_point);
    }

    g_object_notify (G_OBJECT (self), "mount-point");
  }
}

char const *
mpd_storage_tile_get_title (MpdStorageTile *self)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_TILE (self), NULL);

  return mx_label_get_text (MX_LABEL (priv->title));
}

static void
mpd_storage_tile_set_title (MpdStorageTile *self,
                            char const     *title)
{
  MpdStorageTilePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_STORAGE_TILE (self));

  if (0 != g_strcmp0 (title, mx_label_get_text (MX_LABEL (priv->title))))
  {
    mx_label_set_text (MX_LABEL (priv->title), title);

    g_object_notify (G_OBJECT (self), "title");
  }
}



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
#include <gtk/gtk.h>
#include <libnotify/notify.h>

#include "mpd-gobject.h"
#include "mpd-media-import-tile.h"
#include "mpd-shell-defines.h"
#include "mpd-storage-device.h"
#include "mpd-storage-device-tile.h"
#include "mpd-text.h"
#include "config.h"

static void
mpd_storage_device_tile_set_icon_file (MpdStorageDeviceTile  *self,
                                       char const            *icon_file);
static void
mpd_storage_device_tile_set_mime_type (MpdStorageDeviceTile *self,
                                       char const           *mime_type);

static void
mpd_storage_device_tile_set_mount_point (MpdStorageDeviceTile  *self,
                                         char const            *mount_point);

static void
mpd_storage_device_tile_set_name (MpdStorageDeviceTile *self,
                                  char const           *name);

static void
mpd_storage_device_tile_set_title (MpdStorageDeviceTile *self,
                                   char const           *title);

G_DEFINE_TYPE (MpdStorageDeviceTile, mpd_storage_device_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                MPD_TYPE_STORAGE_DEVICE_TILE, \
                                MpdStorageDeviceTilePrivate))

enum
{
  PROP_0,

  PROP_ICON_FILE,
  PROP_MIME_TYPE,
  PROP_MOUNT_POINT,
  PROP_NAME,
  PROP_TITLE
};

enum
{
  REQUEST_HIDE,
  REQUEST_SHOW,
  EJECT,

  LAST_SIGNAL
};

typedef struct
{
  /* Managed by clutter */
  ClutterActor              *icon;
  ClutterActor              *vbox;
  ClutterActor              *label;
  ClutterActor              *meter;
  ClutterActor              *button_box;
  /* Inside button_box. */
  ClutterActor              *eject;
  ClutterActor              *open;
  ClutterActor              *import;

  /* Data */
  char                      *icon_file;
  char                      *mime_type;
  char                      *mount_point;
  char                      *name;
  MpdStorageDevice          *storage;
  bool                       storage_has_media;
} MpdStorageDeviceTilePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
update (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  char          *markup;
  uint64_t       size;
  uint64_t       available_size;
  unsigned int   percentage;

  markup = mpd_storage_device_tile_get_title (self);
  clutter_text_set_markup (GET_CLUTTER_TEXT (priv->label),
                           markup);
  g_free (markup);

  g_object_get (priv->storage,
                "size", &size,
                "available-size", &available_size,
                NULL);

  percentage = 100 - (double) available_size / size * 100;
  mx_progress_bar_set_progress (MX_PROGRESS_BAR (priv->meter),
                                percentage / 100.);
}

static void
_storage_size_notify_cb (MpdStorageDevice     *storage,
                         GParamSpec           *pspec,
                         MpdStorageDeviceTile *self)
{
  update (self);
}

static void
_show_panel_cb (NotifyNotification    *notification,
                gchar                 *action,
                MpdStorageDeviceTile  *self)
{
  g_debug ("%s()", __FUNCTION__);
  g_signal_emit_by_name (self, "request-show");
}

static void
_import_clicked_cb (MxButton             *button,
                    MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  char    *command_line;
  GError  *error = NULL;

  if (0 == g_strcmp0 ("x-content/image-dcf", priv->mime_type))
  {
    command_line = g_strdup ("gthumb --import-photos");
  } else {
    command_line = g_strdup_printf ("banshee --import-media %s",
                                    priv->mount_point);
  }

  g_spawn_command_line_async (command_line, &error);
  if (error)
  {
    char *message = g_strdup_printf (_("Could not run %s"), command_line);
    NotifyNotification *note = notify_notification_new (_("Import error"),
                                                        message,
                                                        NULL,
                                                        NULL);
    notify_notification_set_urgency (note, NOTIFY_URGENCY_CRITICAL);
    notify_notification_show (note, NULL);
    g_object_unref (note);
    g_free (message);
    g_clear_error (&error);
  }

  g_free (command_line);  
}

static void
_eject_clicked_cb (MxButton             *button,
                   MpdStorageDeviceTile *self)
{
  g_signal_emit_by_name (self, "eject");
  g_signal_emit_by_name (self, "request-hide");
}

static void
_open_clicked_cb (MxButton              *button,
                  MpdStorageDeviceTile  *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  char    *uri;
  GError  *error = NULL;

  uri = g_filename_to_uri (priv->mount_point, NULL, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  } else {
    gtk_show_uri (NULL, uri, GDK_CURRENT_TIME, &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
  }
  g_free (uri);

  g_signal_emit_by_name (self, "request-hide");
}

static GObject *
_constructor (GType                  type,
              unsigned int           n_properties,
              GObjectConstructParam *properties)
{
  MpdStorageDeviceTile *self = (MpdStorageDeviceTile *)
                        G_OBJECT_CLASS (mpd_storage_device_tile_parent_class)
                          ->constructor (type, n_properties, properties);
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  NotifyNotification  *notification;
  char                *body;
  GError              *error = NULL;

  if (priv->icon_file &&
      g_file_test (priv->icon_file, G_FILE_TEST_IS_REGULAR))
  {
    clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon),
                                   priv->icon_file,
                                   &error);
  } else {
    clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon),
                                   PKGICONDIR "/device-usb.png",
                                   &error);
  }
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  if (priv->mount_point)
  {
    priv->storage = mpd_storage_device_new (priv->mount_point);
    g_signal_connect (priv->storage, "notify::size",
                      G_CALLBACK (_storage_size_notify_cb), self);
    g_signal_connect (priv->storage, "notify::available-size",
                      G_CALLBACK (_storage_size_notify_cb), self);
    update (self);
  } else {
    g_critical ("%s : %s",
                G_STRLOC,
                "Invalid or no mount-point passed to constructor.");
    self = NULL;
  }

  body = g_strdup_printf (_("%s nas been plugged in. "
                            "You can use the Devices panel interact with it"),
                          priv->name);
  notification = notify_notification_new (_("USB plugged in"), body, NULL, NULL);
  notify_notification_add_action (notification, "show", _("Show"),
                                  (NotifyActionCallback) _show_panel_cb, self,
                                  NULL);
  notify_notification_show (notification, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
  g_object_unref (notification);

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
  case PROP_ICON_FILE:
    g_value_set_string (value,
                        mpd_storage_device_tile_get_icon_file (
                          MPD_STORAGE_DEVICE_TILE (object)));
    break;
  case PROP_MIME_TYPE:
    g_value_set_string (value,
                        mpd_storage_device_tile_get_mime_type (
                          MPD_STORAGE_DEVICE_TILE (object)));
    break;
  case PROP_MOUNT_POINT:
    g_value_set_string (value,
                        mpd_storage_device_tile_get_mount_point (
                          MPD_STORAGE_DEVICE_TILE (object)));
    break;
  case PROP_NAME:
    g_value_set_string (value,
                        mpd_storage_device_tile_get_name (
                          MPD_STORAGE_DEVICE_TILE (object)));
    break;
  case PROP_TITLE:
    g_value_take_string (value,
                         mpd_storage_device_tile_get_title (
                          MPD_STORAGE_DEVICE_TILE (object)));
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
  case PROP_ICON_FILE:
    mpd_storage_device_tile_set_icon_file (MPD_STORAGE_DEVICE_TILE (object),
                                           g_value_get_string (value));
    break;
  case PROP_MIME_TYPE:
    mpd_storage_device_tile_set_mime_type (MPD_STORAGE_DEVICE_TILE (object),
                                           g_value_get_string (value));
    break;
  case PROP_MOUNT_POINT:
    mpd_storage_device_tile_set_mount_point (MPD_STORAGE_DEVICE_TILE (object),
                                             g_value_get_string (value));
    break;
  case PROP_NAME:
    mpd_storage_device_tile_set_name (MPD_STORAGE_DEVICE_TILE (object),
                                      g_value_get_string (value));
    break;
  case PROP_TITLE:
    mpd_storage_device_tile_set_title (MPD_STORAGE_DEVICE_TILE (object),
                                       g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (object);

  if (priv->icon_file)
  {
    g_free (priv->icon_file);
    priv->icon_file = NULL;
  }

  if (priv->mime_type)
  {
    g_free (priv->mime_type);
    priv->mime_type = NULL;
  }

  if (priv->mount_point)
  {
    g_free (priv->mount_point);
    priv->mount_point = NULL;
  }

  if (priv->name)
  {
    g_free (priv->name);
    priv->name = NULL;
  }

  mpd_gobject_detach (object, (GObject **) &priv->storage);

  G_OBJECT_CLASS (mpd_storage_device_tile_parent_class)->dispose (object);
}

static void
mpd_storage_device_tile_class_init (MpdStorageDeviceTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdStorageDeviceTilePrivate));

  object_class->constructor = _constructor;
  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;

  /* Properties */

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_ICON_FILE,
                                   g_param_spec_string ("icon-file",
                                                        "Icon file",
                                                        "Icon file path",
                                                        NULL,
                                                        param_flags |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_MIME_TYPE,
                                   g_param_spec_string ("mime-type",
                                                        "Mime type",
                                                        "Device mime type",
                                                        NULL,
                                                        param_flags |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_MOUNT_POINT,
                                   g_param_spec_string ("mount-point",
                                                        "Mount point",
                                                        "Device mount point",
                                                        NULL,
                                                        param_flags |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "Device name",
                                                        NULL,
                                                        param_flags |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "Tile title",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  /* Signals */

  _signals[REQUEST_HIDE] = g_signal_new ("request-hide",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);

  _signals[REQUEST_SHOW] = g_signal_new ("request-show",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);

  _signals[EJECT] = g_signal_new ("eject",
                                  G_TYPE_FROM_CLASS (klass),
                                  G_SIGNAL_RUN_LAST,
                                  0, NULL, NULL,
                                  g_cclosure_marshal_VOID__VOID,
                                  G_TYPE_NONE, 0);
}

static void
mpd_storage_device_tile_init (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor  *hbox;
  ClutterActor  *separator;

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);

  hbox = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (hbox),
                             MPD_STORAGE_DEVICE_TILE_SPACING);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), hbox);

  /* 1st column: icon */
  priv->icon = clutter_texture_new ();
  clutter_actor_set_size (priv->icon,
                          MPD_STORAGE_DEVICE_TILE_ICON_SIZE,
                          MPD_STORAGE_DEVICE_TILE_ICON_SIZE);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), priv->icon);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), priv->icon,
                               "expand", false,
                               "x-align", MX_ALIGN_START,
                               "x-fill", false,
                               "y-align", MX_ALIGN_START,
                               "y-fill", false,
                               NULL);

  /* 2nd column: text, free space */
  priv->vbox = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->vbox),
                             MPD_STORAGE_DEVICE_TILE_VBOX_SPACING);
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->vbox),
                                 MX_ORIENTATION_VERTICAL);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), priv->vbox);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), priv->vbox,
                               "expand", true,
                               "x-align", MX_ALIGN_START,
                               "x-fill", true,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", false,
                               NULL);

  priv->label = mx_label_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                               priv->label);

  /* Progress bar */
  priv->meter = mx_progress_bar_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                               priv->meter);

  /* Button box */
  priv->button_box = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->button_box),
                             MPD_STORAGE_DEVICE_TILE_BUTTON_BOX_SPACING);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                               priv->button_box);

  priv->import = mx_button_new_with_label (_("Import data"));
  g_signal_connect (priv->import, "clicked",
                    G_CALLBACK (_import_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                                priv->import);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->button_box),
                                priv->import,
                                "expand", true,
                                "x-fill", true,
                                NULL);

  /* Eject button */
  priv->eject = mx_button_new_with_label (_("Eject"));
  g_signal_connect (priv->eject, "clicked",
                    G_CALLBACK (_eject_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                               priv->eject);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->button_box),
                               priv->eject,
                               "expand", true,
                               "x-fill", true,
                               NULL);

  /* Open button */
  priv->open = mx_button_new_with_label (_("Open"));
  g_signal_connect (priv->open, "clicked",
                    G_CALLBACK (_open_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                               priv->open);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->button_box),
                               priv->open,
                               "expand", true,
                               "x-fill", true,
                               NULL);

  /* Separator */
  separator = mx_icon_new ();
  clutter_actor_set_height (separator, 1.0);
  mx_stylable_set_style_class (MX_STYLABLE (separator), "separator");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), separator);
}

ClutterActor *
mpd_storage_device_tile_new (char const *name,
                             char const *mount_point,
                             char const *mime_type,
                             char const *icon_file)
{
  return g_object_new (MPD_TYPE_STORAGE_DEVICE_TILE,
                       "icon-file", icon_file,
                       "mime-type", mime_type,
                       "mount-point", mount_point,
                       "name", name,
                       NULL);
}

char const *
mpd_storage_device_tile_get_icon_file (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self), NULL);

  return priv->icon_file;
}

static void
mpd_storage_device_tile_set_icon_file (MpdStorageDeviceTile  *self,
                                       char const            *icon_file)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self));

  if (0 != g_strcmp0 (icon_file, priv->icon_file))
  {
    if (priv->icon_file)
    {
      g_free (priv->icon_file);
      priv->icon_file = NULL;
    }

    if (icon_file)
    {
      priv->icon_file = g_strdup (icon_file);
    }

    g_object_notify (G_OBJECT (self), "icon-file");
  }
}

char const *
mpd_storage_device_tile_get_mime_type (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self), NULL);

  return priv->mime_type;
}

static void
mpd_storage_device_tile_set_mime_type (MpdStorageDeviceTile *self,
                                       char const           *mime_type)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self));

  if (0 != g_strcmp0 (mime_type, priv->mime_type))
  {
    if (priv->mime_type)
    {
      g_free (priv->mime_type);
      priv->mime_type = NULL;
    }

    if (mime_type)
    {
      priv->mime_type = g_strdup (mime_type);
    }

    g_object_notify (G_OBJECT (self), "mime-type");
  }
}

char const *
mpd_storage_device_tile_get_mount_point (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self), NULL);

  return priv->mount_point;
}

static void
mpd_storage_device_tile_set_mount_point (MpdStorageDeviceTile *self,
                                         char const           *mount_point)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self));

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
mpd_storage_device_tile_get_name (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self), NULL);

  return priv->name;
}

static void
mpd_storage_device_tile_set_name (MpdStorageDeviceTile *self,
                                  char const           *name)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self));

  if (0 != g_strcmp0 (name, priv->name))
  {
    if (priv->name)
    {
      g_free (priv->name);
      priv->name = NULL;
    }

    if (name)
    {
      priv->name = g_strdup (name);
    }

    g_object_notify (G_OBJECT (self), "name");
  }
}

char *
mpd_storage_device_tile_get_title (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  char          *markup;
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
  markup = g_strdup_printf (_("<span font-weight='bold'>%s</span> using "
                              "<span color='%s'>%d%% of %s</span>"),
                              priv->name,
                              TEXT_COLOR,
                              percentage,
                              size_text);
  g_free (size_text);

  return markup;
}

static void
mpd_storage_device_tile_set_title (MpdStorageDeviceTile *self,
                                   char const           *title)
{
  /* Setting title only when changing state. */
  g_warning ("%s() not implemented", __FUNCTION__);
}

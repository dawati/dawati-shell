
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
  PROP_MOUNT_POINT,
  PROP_NAME,
  PROP_STATE,
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

  MpdStorageDeviceTileState  state;
  union {
    struct {
      ClutterActor          *label;
      ClutterActor          *eject;
    } searching;
    struct {
      ClutterActor          *label;
      ClutterActor          *meter;
      ClutterActor          *button_box;
      /* Inside button_box. */
      ClutterActor          *eject;
      ClutterActor          *open;
      ClutterActor          *import;
      /* During import. */
      ClutterActor          *import_tile;
    } ready;
    struct {
      ClutterActor          *label;
    } unmounting;
    struct {
      ClutterActor          *label;
    } done;
  } states;

  /* Data */
  char                      *icon_file;
  char                      *mount_point;
  char                      *name;
  MpdStorageDevice          *storage;
  bool                       storage_has_media;
} MpdStorageDeviceTilePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static ClutterActor *
description_label_new (void)
{
  ClutterActor  *label;
  ClutterText   *text;

  label = mx_label_new ();
  text = CLUTTER_TEXT (mx_label_get_clutter_text (MX_LABEL (label)));
  clutter_text_set_ellipsize (text, PANGO_ELLIPSIZE_NONE);
  clutter_text_set_line_wrap_mode (text, PANGO_WRAP_WORD);

  return label;
}

static void
update (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  char          *markup;
  uint64_t       size;
  uint64_t       available_size;
  unsigned int   percentage;

  g_return_if_fail (priv->state == STATE_READY);

  markup = mpd_storage_device_tile_get_title (self);
  clutter_text_set_markup (GET_CLUTTER_TEXT (priv->states.ready.label),
                           markup);
  g_free (markup);

  g_object_get (priv->storage,
                "size", &size,
                "available-size", &available_size,
                NULL);

  percentage = 100 - (double) available_size / size * 100;
  mx_progress_bar_set_progress (MX_PROGRESS_BAR (priv->states.ready.meter),
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
show_import_error (MpdStorageDeviceTile *self,
                   GError const         *error)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  NotifyNotification  *note;
  char const          *summary;
  char                *body;
  GError              *note_error = NULL;

  g_return_if_fail (error);

  switch (error->code)
  {
  case MPD_STORAGE_DEVICE_IMPORT_ERROR_STILL_INDEXING:
    summary = _("Import cancelled");
    body = g_strdup_printf (_("We are still looking for media files on \"%s\""),
                            priv->name);
    break;
  case MPD_STORAGE_DEVICE_IMPORT_ERROR_NO_MEDIA:
    summary = _("Import cancelled");
    body = g_strdup_printf (_("We did not find any media files on \"%s\""),
                            priv->name);
    break;
  case MPD_STORAGE_DEVICE_IMPORT_ERROR_INSUFICCIENT_DISK_SPACE:
    summary = _("Import cancelled");
    body = g_strdup_printf (_("Sorry we could not import your media, "
                              "there is not enough space on your computer. "
                              "You could remove some files."));
    break;
  default:
    summary = _("Import cancelled");
    body = g_strdup (error->message);
  }

  note = notify_notification_new (summary, body, NULL, NULL);
  notify_notification_add_action (note, "show", _("Show"),
                                  (NotifyActionCallback) _show_panel_cb, self,
                                  NULL);
  notify_notification_show (note, &note_error);
  if (note_error)
  {
    g_warning ("%s : %s", G_STRLOC, note_error->message);
    g_clear_error (&note_error);
  }
  g_object_unref (note);

  g_free (body);
}

static void
stop_import (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  mpd_storage_device_stop_import (priv->storage);

  clutter_actor_destroy (priv->states.ready.import_tile);

  clutter_actor_set_reactive (priv->states.ready.import, true);
  clutter_actor_set_reactive (priv->states.ready.eject, true);
  clutter_actor_set_reactive (priv->states.ready.open, true);
}

static void
_import_error_cb (MpdStorageDevice      *storage,
                  GError const          *error,
                  MpdStorageDeviceTile  *self)
{
  show_import_error (self, error);
  stop_import (self);
}

static void
_import_stop_cb (MpdMediaImportTile   *import,
                 MpdStorageDeviceTile *self)
{
  stop_import (self);
}

static void
_import_clicked_cb (MxButton             *button,
                    MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  g_signal_connect (priv->storage, "import-error",
                    G_CALLBACK (_import_error_cb), self);

  priv->states.ready.import_tile = mpd_media_import_tile_new (priv->storage);
  g_signal_connect (priv->states.ready.import_tile, "stop-import",
                    G_CALLBACK (_import_stop_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                               priv->states.ready.import_tile);

  clutter_actor_set_reactive (priv->states.ready.import, false);
  clutter_actor_set_reactive (priv->states.ready.eject, false);
  clutter_actor_set_reactive (priv->states.ready.open, false);

  mpd_storage_device_import_async (priv->storage, &error);
  if (error)
  {
    show_import_error (self, error);
    g_clear_error (&error);
  }
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

static void
_storage_has_media_cb (MpdStorageDevice     *storage,
                       bool                  has_media,
                       MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (priv->state == STATE_SEARCHING);

  priv->storage_has_media = has_media;
  mpd_storage_device_tile_set_state (self, STATE_READY);
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
    mpd_storage_device_tile_set_state (self, STATE_SEARCHING);
    g_signal_connect (priv->storage, "has-media",
                      G_CALLBACK (_storage_has_media_cb), self);
    mpd_storage_device_has_media_async (priv->storage);

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
  case PROP_STATE:
    g_value_set_int (value,
                     mpd_storage_device_tile_get_state (
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
  case PROP_MOUNT_POINT:
    mpd_storage_device_tile_set_mount_point (MPD_STORAGE_DEVICE_TILE (object),
                                             g_value_get_string (value));
    break;
  case PROP_NAME:
    mpd_storage_device_tile_set_name (MPD_STORAGE_DEVICE_TILE (object),
                                      g_value_get_string (value));
    break;
  case PROP_STATE:
    mpd_storage_device_tile_set_state (MPD_STORAGE_DEVICE_TILE (object),
                                       g_value_get_int (value));
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
                                   PROP_STATE,
                                   g_param_spec_int ("state",
                                                     "State",
                                                     "Tile state",
                                                     STATE_ERROR,
                                                     STATE_DONE,
                                                     STATE_ERROR,
                                                     param_flags));
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

  priv->state = STATE_ERROR;

  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self),
                             MPD_STORAGE_DEVICE_TILE_SPACING);

  /* 1st column: icon */
  priv->icon = clutter_texture_new ();
  clutter_actor_set_size (priv->icon,
                          MPD_STORAGE_DEVICE_TILE_ICON_SIZE,
                          MPD_STORAGE_DEVICE_TILE_ICON_SIZE);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->icon);
  clutter_container_child_set (CLUTTER_CONTAINER (self), priv->icon,
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
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->vbox);
  clutter_container_child_set (CLUTTER_CONTAINER (self), priv->vbox,
                               "expand", true,
                               "x-align", MX_ALIGN_START,
                               "x-fill", true,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", false,
                               NULL);
}

ClutterActor *
mpd_storage_device_tile_new (char const *name,
                             char const *mount_point,
                             char const *icon_file)
{
  return g_object_new (MPD_TYPE_STORAGE_DEVICE_TILE,
                       "icon-file", icon_file,
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

MpdStorageDeviceTileState
mpd_storage_device_tile_get_state (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self), STATE_ERROR);

  return priv->state;
}

void
mpd_storage_device_tile_set_state (MpdStorageDeviceTile       *self,
                                   MpdStorageDeviceTileState   state)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  char *label;

  g_return_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self));
  g_return_if_fail (state == priv->state + 1);

  g_debug ("%s() %d", __FUNCTION__, state);

  priv->state = state;
  switch (priv->state)
  {
  case STATE_ERROR:
    /* TODO */
    break;
  case STATE_SEARCHING:

    /* Description */
    priv->states.searching.label = description_label_new ();
    mx_label_set_text (MX_LABEL (priv->states.searching.label),
                       _("Please wait a moment while we search for data on the device ..."));
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                                 priv->states.searching.label);

    /* Eject button */
    priv->states.searching.eject = mx_button_new_with_label (_("Eject"));
    g_signal_connect (priv->states.searching.eject, "clicked",
                      G_CALLBACK (_eject_clicked_cb), self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                                 priv->states.searching.eject);
    clutter_container_child_set (CLUTTER_CONTAINER (priv->vbox),
                                 priv->states.searching.eject,
                                 "expand", false,
                                 "x-align", MX_ALIGN_END,
                                 NULL);

    /* TODO search */

    break;

  case STATE_READY:

    /* Cleanup state "searching". */
    clutter_actor_destroy (priv->states.searching.label);
    clutter_actor_destroy (priv->states.searching.eject);

    memset (&priv->states.searching, 0, sizeof (priv->states.searching));

    /* "Description" is set in update() below. */
    priv->states.ready.label = description_label_new ();
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                                 priv->states.searching.label);

    /* Progress bar */
    priv->states.ready.meter = mx_progress_bar_new ();
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                                 priv->states.ready.meter);

    /* Button box */
    priv->states.ready.button_box = mx_box_layout_new ();
    mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->states.ready.button_box),
                               MPD_STORAGE_DEVICE_TILE_BUTTON_BOX_SPACING);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                                 priv->states.ready.button_box);

    /* Copy button */
    if (priv->storage_has_media)
    {
      priv->states.ready.import = mx_button_new_with_label (_("Import data"));
      g_signal_connect (priv->states.ready.import, "clicked",
                        G_CALLBACK (_import_clicked_cb), self);
      clutter_container_add_actor (CLUTTER_CONTAINER (priv->states.ready.button_box),
                                   priv->states.ready.import);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->states.ready.button_box),
                                   priv->states.ready.import,
                                   "expand", true,
                                   "x-fill", true,
                                   NULL);
    }

    /* Eject button */
    priv->states.ready.eject = mx_button_new_with_label (_("Eject"));
    g_signal_connect (priv->states.ready.eject, "clicked",
                      G_CALLBACK (_eject_clicked_cb), self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->states.ready.button_box),
                                 priv->states.ready.eject);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->states.ready.button_box),
                                   priv->states.ready.eject,
                                   "expand", true,
                                   "x-fill", true,
                                   NULL);

    /* Open button */
    priv->states.ready.open = mx_button_new_with_label (_("Open"));
    g_signal_connect (priv->states.ready.open, "clicked",
                      G_CALLBACK (_open_clicked_cb), self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->states.ready.button_box),
                                 priv->states.ready.open);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->states.ready.button_box),
                                   priv->states.ready.open,
                                   "expand", true,
                                   "x-fill", true,
                                   NULL);

    g_signal_connect (priv->storage, "notify::size",
                      G_CALLBACK (_storage_size_notify_cb), self);
    g_signal_connect (priv->storage, "notify::available-size",
                      G_CALLBACK (_storage_size_notify_cb), self);

    update (self);

    break;

  case STATE_UNMOUNTING:

    /* Cleanup state "ready". */
    clutter_actor_destroy (priv->states.ready.label);
    clutter_actor_destroy (priv->states.ready.meter);
    clutter_actor_destroy (priv->states.ready.button_box);

    /* Description */
    label = g_strdup_printf (_("Please wait whilst your %s is being ejected ..."),
                             priv->name);
    priv->states.unmounting.label = description_label_new ();
    mx_label_set_text (MX_LABEL (priv->states.unmounting.label), label);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                                 priv->states.unmounting.label);
    g_free (label);

    break;

  case STATE_DONE:

    /* Cleanup state "unmounting". */
    clutter_actor_destroy (priv->states.unmounting.label);

    /* Description */
    label = g_strdup_printf (_("It is safe to pull out your %s"),
                             priv->name);
    priv->states.done.label = description_label_new ();
    mx_label_set_text (MX_LABEL (priv->states.done.label), label);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->vbox),
                                 priv->states.done.label);
    g_free (label);

    break;
  }
  g_object_notify (G_OBJECT (self), "state");
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

  g_return_val_if_fail (priv->state == STATE_READY, NULL);

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

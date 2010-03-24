
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

#include "mpd-gobject.h"
#include "mpd-shell-defines.h"
#include "mpd-storage-device.h"
#include "mpd-storage-device-tile.h"
#include "config.h"

static void
mpd_storage_device_tile_set_icon_file (MpdStorageDeviceTile  *self,
                                       char const            *icon_file);
static void
mpd_storage_device_tile_set_mount_point (MpdStorageDeviceTile  *self,
                                         char const            *mount_point);

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
  PROP_STATE,
  PROP_TITLE
};

enum
{
  REQUEST_HIDE,
  EJECT,

  LAST_SIGNAL
};

typedef struct
{
  /* Managed by clutter */
  ClutterActor              *icon;
  ClutterActor              *middle_col;
  ClutterActor              *right_col;

  MpdStorageDeviceTileState  state;
  union {
    struct {
      ClutterActor          *title;
      ClutterActor          *description;
      ClutterActor          *eject;
    } searching;
    struct {
      ClutterActor          *title;
      ClutterActor          *description;
      ClutterActor          *meter;
      ClutterActor          *eject;
      ClutterActor          *open;
      ClutterActor          *copy;
    } ready;
    struct {
      ClutterActor          *description;
    } unmounting;
    struct {
      ClutterActor          *description;
    } done;
  } states;

  /* Data */
  char                      *icon_file;
  char                      *mount_point;
  MpdStorageDevice          *storage;
} MpdStorageDeviceTilePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
assign_title (MpdStorageDeviceTile  *self,
              MxLabel               *label)
{
  char *title;

  title = mpd_storage_device_tile_get_title (self);
  mx_label_set_text (label, title);
  g_free (title);
}

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
  char          *text;
  char          *size_text;
  uint64_t       size;
  uint64_t       available_size;
  unsigned int   percentage;

  g_return_if_fail (priv->state == STATE_READY);

  g_object_get (priv->storage,
                "size", &size,
                "available-size", &available_size,
                NULL);

  percentage = 100 - (double) available_size / size * 100;

  size_text = g_format_size_for_display (size);
  text = g_strdup_printf (_("Using %d%% of %s"), percentage, size_text);
  mx_label_set_text (MX_LABEL (priv->states.ready.description), text);
  g_free (text);
  g_free (size_text);

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
_copy_clicked_cb (MxButton             *button,
                  MpdStorageDeviceTile *self)
{
  // TODO
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
  GError *error = NULL;

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

  } else {
    g_critical ("%s : %s",
                G_STRLOC,
                "Invalid or no mount-point passed to constructor.");
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
                                   PROP_STATE,
                                   g_param_spec_int ("state",
                                                     "State",
                                                     "Tile state",
                                                     STATE_PRISTINE,
                                                     STATE_DONE,
                                                     STATE_PRISTINE,
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
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", false,
                               NULL);

  /* 2nd column: text, free space */
  priv->middle_col = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->middle_col),
                                 MX_ORIENTATION_VERTICAL);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->middle_col);
  clutter_container_child_set (CLUTTER_CONTAINER (self), priv->middle_col,
                               "expand", true,
                               "x-align", MX_ALIGN_START,
                               "x-fill", true,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", false,
                               NULL);

  /* 3rd column: buttons */
  priv->right_col = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->right_col),
                                 MX_ORIENTATION_VERTICAL);
  clutter_container_add_actor (CLUTTER_CONTAINER (self),
                               priv->right_col);
  clutter_container_child_set (CLUTTER_CONTAINER (self), priv->right_col,
                               "expand", false,
                               "x-align", MX_ALIGN_END,
                               "x-fill", false,
                               "y-align", MX_ALIGN_START,
                               "y-fill", true,
                               NULL);
}

ClutterActor *
mpd_storage_device_tile_new (char const *mount_point,
                             char const *icon_file)
{
  return g_object_new (MPD_TYPE_STORAGE_DEVICE_TILE,
                       "mount-point", mount_point,
                       "icon-file", icon_file,
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
  char *title;
  char *description;

  g_return_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self));
  g_return_if_fail (state == priv->state + 1);

  g_debug ("%s() %d", __FUNCTION__, state);

  priv->state = state;
  switch (priv->state)
  {
  case STATE_ERROR:
    /* TODO */
    break;
  case STATE_PRISTINE:
    /* Initial state, nothing to do. */
    break;
  case STATE_SEARCHING:

    /* Title */
    priv->states.searching.title = mx_label_new ();
    assign_title (self, MX_LABEL (priv->states.searching.title));
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->middle_col),
                                 priv->states.searching.title);

    /* Description */
    priv->states.searching.description = description_label_new ();
    mx_label_set_text (MX_LABEL (priv->states.searching.description),
                       _("Please wait a moment while we search for data on the device ..."));
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->middle_col),
                                 priv->states.searching.description);

    /* Eject button */
    priv->states.searching.eject = mx_button_new_with_label (_("Eject"));
    g_signal_connect (priv->states.searching.eject, "clicked",
                      G_CALLBACK (_eject_clicked_cb), self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->right_col),
                                 priv->states.searching.eject);

    /* TODO search */

    break;

  case STATE_READY:

    /* Cleanup state "searching". */
    clutter_actor_destroy (priv->states.searching.title);
    clutter_actor_destroy (priv->states.searching.description);
    clutter_actor_destroy (priv->states.searching.eject);

    /* Title */
    priv->states.ready.title = mx_label_new ();
    assign_title (self, MX_LABEL (priv->states.ready.title));
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->middle_col),
                                 priv->states.ready.title);

    /* Description */
    priv->states.ready.description = description_label_new ();
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->middle_col),
                                 priv->states.ready.description);

    /* Progress bar */
    priv->states.ready.meter = mx_progress_bar_new ();
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->middle_col),
                                 priv->states.ready.meter);

    /* Copy button */
    priv->states.ready.copy = mx_button_new_with_label (
                                _("Copy media to your computer"));
    g_signal_connect (priv->states.ready.copy, "clicked",
                      G_CALLBACK (_copy_clicked_cb), self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->middle_col),
                                 priv->states.ready.copy);
    clutter_container_child_set (CLUTTER_CONTAINER (priv->middle_col),
                                 priv->states.ready.copy,
                                 "expand", false,
                                 "x-align", MX_ALIGN_END,
                                 "x-fill", false,
                                 "y-align", MX_ALIGN_MIDDLE,
                                 "y-fill", false,
                                 NULL);

    /* Eject button */
    priv->states.ready.eject = mx_button_new_with_label (_("Eject"));
    g_signal_connect (priv->states.ready.eject, "clicked",
                      G_CALLBACK (_eject_clicked_cb), self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->right_col),
                                 priv->states.ready.eject);

    /* Open button */
    priv->states.ready.open = mx_button_new_with_label (_("Open"));
    g_signal_connect (priv->states.ready.eject, "clicked",
                      G_CALLBACK (_open_clicked_cb), self);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->right_col),
                                 priv->states.ready.open);
    clutter_container_child_set (CLUTTER_CONTAINER (priv->middle_col),
                                 priv->states.ready.open,
                                 "expand", false,
                                 "x-align", MX_ALIGN_END,
                                 "x-fill", false,
                                 "y-align", MX_ALIGN_END,
                                 "y-fill", false,
                                 NULL);

    g_signal_connect (priv->storage, "notify::size",
                      G_CALLBACK (_storage_size_notify_cb), self);
    g_signal_connect (priv->storage, "notify::available-size",
                      G_CALLBACK (_storage_size_notify_cb), self);

    update (self);

    break;

  case STATE_UNMOUNTING:

    /* Cleanup state "ready". */
    clutter_actor_destroy (priv->states.ready.title);
    clutter_actor_destroy (priv->states.ready.description);
    clutter_actor_destroy (priv->states.ready.meter);
    clutter_actor_destroy (priv->states.ready.copy);
    clutter_actor_destroy (priv->states.ready.eject);
    clutter_actor_destroy (priv->states.ready.open);

    /* Description */
    title = mpd_storage_device_tile_get_title (self);
    description = g_strdup_printf (_("Please wait whilst your %s is being ejected ..."),
                                   title);
    priv->states.unmounting.description = description_label_new ();
    mx_label_set_text (MX_LABEL (priv->states.unmounting.description),
                       description);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->middle_col),
                                 priv->states.unmounting.description);
    g_free (description);
    g_free (title);

    break;

  case STATE_DONE:

    /* Cleanup state "unmounting". */
    clutter_actor_destroy (priv->states.unmounting.description);

    /* Description */
    title = mpd_storage_device_tile_get_title (self);
    description = g_strdup_printf (_("It is safe to pull out your %s"),
                                   title);
    priv->states.done.description = description_label_new ();
    mx_label_set_text (MX_LABEL (priv->states.done.description),
                       description);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->middle_col),
                                 priv->states.done.description);
    g_free (description);
    g_free (title);

    break;
  }
  g_object_notify (G_OBJECT (self), "state");
}

char *
mpd_storage_device_tile_get_title (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  char const *label;
  char const *model;
  char const *vendor;
  char *title;

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self), NULL);

  label = mpd_storage_device_get_label (priv->storage);
  model = mpd_storage_device_get_model (priv->storage);
  vendor = mpd_storage_device_get_vendor (priv->storage);

#ifdef HAVE_UDISKS
  if (label && *label)
    title = g_strdup_printf ("%s - %s %s", label, vendor, model);
  else
    title = g_strdup_printf ("%s %s", vendor, model);
#else
  title = g_path_get_basename (priv->mount_point);
#endif

  return title;
}

static void
mpd_storage_device_tile_set_title (MpdStorageDeviceTile *self,
                                   char const           *title)
{
  /* Setting title only when changing state. */
  g_warning ("%s() not implemented", __FUNCTION__);
}

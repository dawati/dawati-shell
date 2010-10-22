
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
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>

#include "mpd-gobject.h"
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
  ClutterActor              *table;
  ClutterActor              *icon;
  ClutterActor              *label;
  ClutterActor              *meter;
  ClutterActor              *eject;
  ClutterActor              *open;
  ClutterActor              *import;
  ClutterActor              *message;

  /* Data */
  char                      *icon_file;
  char                      *mime_type;
  char                      *mount_point;
  char                      *name;
  MpdStorageDevice          *storage;
  bool                       storage_has_media;
} MpdStorageDeviceTilePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

#define MPD_STORAGE_DEVICE_TILE_ERROR (mpd_storage_device_tile_error_quark ())

static GQuark
mpd_storage_device_tile_error_quark (void)
{
  static GQuark _quark = 0;
  if (!_quark)
    _quark = g_quark_from_static_string ("mpd-storage-device-tile-error");
  return _quark;
}

static void
update (MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  char          *markup;
  uint64_t       size;
  uint64_t       available_size = 0;
  unsigned int   percentage;

  markup = mpd_storage_device_tile_get_title (self);
  clutter_text_set_markup (GET_CLUTTER_TEXT (priv->label),
                           markup);
  g_free (markup);

  if (priv->storage)
  {
    g_object_get (priv->storage,
                  "size", &size,
                  "available-size", &available_size,
                  NULL);
    percentage = 100 - (double) available_size / size * 100;
    mx_progress_bar_set_progress (MX_PROGRESS_BAR (priv->meter),
                                  percentage / 100.);
  }

  if (available_size == 0)
  {
    g_object_set (priv->meter, "visible", false, NULL);
    mx_table_child_set_row_span (MX_TABLE (priv->table), priv->label, 2);
  }
}

static void
_storage_size_notify_cb (MpdStorageDevice     *storage,
                         GParamSpec           *pspec,
                         MpdStorageDeviceTile *self)
{
  g_return_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self));

  update (self);
}

static bool
launch_gthumb_import (MpdStorageDeviceTile  *self,
                      GError               **error)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  GAppInfo *appinfo;
  GList     uris = { NULL, };

  appinfo = (GAppInfo *) g_desktop_app_info_new ("gthumb-import.desktop");
  if (NULL == appinfo)
  {
    *error = g_error_new (MPD_STORAGE_DEVICE_TILE_ERROR,
                          0,
                          "%s : Failed to open \"gthumb-import.desktop\"",
                          G_STRLOC);
    return false;
  }

  uris.data = priv->mount_point;
  g_debug ("gthumb-imported.desktop %s", priv->mount_point);

  return g_app_info_launch_uris (appinfo, &uris, NULL, error);
}

static bool
launch_import (MpdStorageDeviceTile   *self,
               char const             *program,
               char const             *args,
               GError                **error)
{
  char  *binary;
  char  *command_line;
  bool   ret;

  binary = g_find_program_in_path (program);
  if (NULL == binary)
  {
    *error = g_error_new (MPD_STORAGE_DEVICE_TILE_ERROR,
                          0,
                          "%s : Failed to find \"%s\" in the path",
                          G_STRLOC,
                          program);
    return false;
  } else
  {
    g_free (binary);
  }

  command_line = g_strconcat (program, " ", args, NULL);
  ret = g_spawn_command_line_async (command_line, error);
  g_free (command_line);
  return ret;
}

static void
_import_clicked_cb (MxButton             *button,
                    MpdStorageDeviceTile *self)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);
  char const  *program = NULL;
  GError      *error = NULL;

  if (0 == g_strcmp0 ("x-content/image-dcf", priv->mime_type))
  {
    /* Photo devices are handled by GThumb.
     * + gphoto2 devices are launched through gthumb-import.desktop with
     *   its Exec hack for exclusive device access.
     * + others (USB cameras) don't work with that hack, so GThumb is
     *   launched in import mode manually. */
    char *scheme = g_uri_parse_scheme (priv->mount_point);
    program = "gthumb";
    if (0 == g_strcmp0 ("gphoto2", scheme))
      launch_gthumb_import (self, &error);
    else
      launch_import (self, program, "--import-photos", &error);
  } else
  {
    program = "banshee-1";
    launch_import (self, program, "--show-import-media", &error);
  }

  if (error)
  {
    NotifyNotification *note;
    char *message = g_strdup_printf (_("Could not run \"%s\""), program);

    note = notify_notification_new (_("Import error"), message, NULL, NULL);
    notify_notification_set_urgency (note, NOTIFY_URGENCY_CRITICAL);

    notify_notification_show (note, NULL);
    g_signal_connect (note, "closed",
                      G_CALLBACK (g_object_unref), NULL);

    g_free (message);
    g_clear_error (&error);
  } else {
    g_signal_emit_by_name (self, "request-hide");
  }
}

static void
_eject_clicked_cb (MxButton             *button,
                   MpdStorageDeviceTile *self)
{
  g_signal_emit_by_name (self, "eject");
}

static void
_open_clicked_cb (MxButton              *button,
                  MpdStorageDeviceTile  *self)
{
  GError *error = NULL;
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  if (!priv->mount_point)
  {
    g_warning ("%s: Mount point uri not set", G_STRLOC);
    return;
  }

  gtk_show_uri (NULL, priv->mount_point,
                clutter_get_current_event_time (), &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

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
  GError  *error = NULL;

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
    char *path;

    path = g_filename_from_uri (priv->mount_point, NULL, &error);
    if (error)
    {
      /* not all uris are file paths, that's ok */
      if (error->code != G_CONVERT_ERROR_BAD_URI)
      {
        g_warning ("%s: Failed to get filename from uri: %s",
                   G_STRLOC, error->message);
      }
      g_clear_error (&error);
    }
    if (path)
    {
      priv->storage = mpd_storage_device_new (path);
      g_signal_connect (priv->storage, "notify::size",
                      G_CALLBACK (_storage_size_notify_cb), self);
      g_signal_connect (priv->storage, "notify::available-size",
                      G_CALLBACK (_storage_size_notify_cb), self);
      g_free (path);
    }

    update (self);

  } else {
    g_critical ("%s : %s",
                G_STRLOC,
                "Invalid or no mount-point passed to constructor.");
    self = NULL;
  }

  /* FIXME: import button should only be active if the import app is available,
   * otherwise show an error. */
  if (0 == g_strcmp0 ("x-content/image-dcf", priv->mime_type))
  {
    mx_button_set_label (MX_BUTTON (priv->import), _("Import photos"));
  } else {
    mx_button_set_label (MX_BUTTON (priv->import), _("Import media"));
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
                                                        "Device mount point uri",
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
  ClutterText   *text;
  ClutterActor  *separator;

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self),
                                 MX_ORIENTATION_VERTICAL);
  mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (self), true);

  priv->table = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (priv->table), 5);
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (self), priv->table, -1,
                                           "x-fill", true,
                                           NULL);
/*
   0      1           2
  +--------------------------+ Table
0 |      | Text      |  Open |
1 | Icon | Progress  | Eject |
  +------+-----------+-------+ Vbox
2 | <message> .. Import data |
  +--------------------------+
3 | ======================== |
  +--------------------------+
*/

  /*
   * Column 0: icon
   */

  priv->icon = clutter_texture_new ();
  clutter_actor_set_size (priv->icon,
                          MPD_STORAGE_DEVICE_TILE_ICON_SIZE,
                          MPD_STORAGE_DEVICE_TILE_ICON_SIZE);
  mx_table_add_actor_with_properties (MX_TABLE (priv->table), priv->icon, 0, 0,
                                      "row-span", 2,
                                      "column-span", 1,
                                      "x-align", MX_ALIGN_START,
                                      "x-expand", false,
                                      "x-fill", false,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-expand", false,
                                      "y-fill", false,
                                      NULL);

  /*
   * Column 1
   */

  /* Text */
  priv->label = mx_label_new ();
  clutter_actor_set_width (priv->label, 200.0);
  text = (ClutterText *) mx_label_get_clutter_text (MX_LABEL (priv->label));
  clutter_text_set_line_wrap (text, true);
  clutter_text_set_line_wrap_mode (text, PANGO_WRAP_WORD);
  clutter_text_set_single_line_mode (text, false);
  clutter_text_set_ellipsize (text, PANGO_ELLIPSIZE_END);
  mx_table_add_actor_with_properties (MX_TABLE (priv->table), priv->label, 0, 1,
                                      "x-align", MX_ALIGN_START,
                                      "x-expand", true,
                                      "x-fill", true,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-expand", false,
                                      "y-fill", false,
                                      NULL);

  /* Progress */
  priv->meter = mx_progress_bar_new ();
  mx_table_add_actor_with_properties (MX_TABLE (priv->table), priv->meter, 1, 1,
                                      "x-align", MX_ALIGN_START,
                                      "x-expand", true,
                                      "x-fill", true,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-expand", false,
                                      "y-fill", false,
                                      NULL);

  /*
   * Column 2: buttons
   */

  /* Open button */
  priv->open = mx_button_new_with_label (_("Open"));
  g_signal_connect (priv->open, "clicked",
                    G_CALLBACK (_open_clicked_cb), self);
  mx_table_add_actor_with_properties (MX_TABLE (priv->table), priv->open, 0, 2,
                                      "x-align", MX_ALIGN_END,
                                      "x-expand", false,
                                      "x-fill", true,
                                      "y-align", MX_ALIGN_END,
                                      "y-expand", false,
                                      "y-fill", false,
                                      NULL);

  /* Eject button */
  priv->eject = mx_button_new_with_label (_("Eject"));
  g_signal_connect (priv->eject, "clicked",
                    G_CALLBACK (_eject_clicked_cb), self);
  mx_table_add_actor_with_properties (MX_TABLE (priv->table), priv->eject, 1, 2,
                                      "x-align", MX_ALIGN_END,
                                      "x-expand", false,
                                      "x-fill", true,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-expand", false,
                                      "y-fill", false,
                                      NULL);

  /*
   * Row 2
   */

  /* Import button */
  priv->import = mx_button_new ();
  clutter_actor_set_name (priv->import, "import");
  g_signal_connect (priv->import, "clicked",
                    G_CALLBACK (_import_clicked_cb), self);
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (self), priv->import,
                                           -1,
                                           "expand", false,
                                           "x-align", MX_ALIGN_END,
                                           "x-fill", false,
                                           NULL);

  /*
   * 4th row: separator
   */

  /* Separator */
  separator = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (separator), "separator");
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (self), separator,
                                           -1,
                                           "expand", false,
                                           "x-align", MX_ALIGN_MIDDLE,
                                           "x-fill", true,
                                           "y-align", MX_ALIGN_START,
                                           "y-fill", false,
                                           NULL);
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
  uint64_t       available_size = 0;
  unsigned int   percentage;

  if (priv->storage)
  {
    g_object_get (priv->storage,
                  "size", &size,
                  "available-size", &available_size,
                  NULL);

    percentage = 100 - (double) available_size / size * 100;
  }

  if (available_size > 0) {
    size_text = g_format_size_for_display (size);
    markup = g_strdup_printf (_("<span font-weight='bold'>%s</span> using "
                                "<span color='%s'>%d%% of %s</span>"),
                              priv->name,
                              TEXT_COLOR,
                              percentage,
                              size_text);
    g_free (size_text);
  } else {
    markup = g_strdup_printf (_("<span font-weight='bold'>%s</span>"),
                              priv->name);
  }

  return markup;
}

static void
mpd_storage_device_tile_set_title (MpdStorageDeviceTile *self,
                                   char const           *title)
{
  /* Setting title only when changing state. */
  g_warning ("%s() not implemented", __FUNCTION__);
}

void
mpd_storage_device_tile_show_message (MpdStorageDeviceTile  *self,
                                      char const            *message,
                                      bool                   replace_buttons)
{
  mpd_storage_device_tile_show_message_full (self,
                                             message,
                                             replace_buttons,
                                             0,
                                             NULL,
                                             NULL);
}

void
mpd_storage_device_tile_show_message_full (MpdStorageDeviceTile  *self,
                                           char const            *message,
                                           bool                   replace_buttons,
                                           unsigned int           timeout_s,
                                           GSourceFunc            function,
                                           void                  *data)
{
  MpdStorageDeviceTilePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_STORAGE_DEVICE_TILE (self));

  if (priv->message == NULL)
  {
    ClutterText *text;
    priv->message = mx_label_new_with_text (message);
    mx_stylable_set_style_class (MX_STYLABLE (priv->message), "message");
    text = (ClutterText *) mx_label_get_clutter_text (MX_LABEL (priv->message));
    clutter_text_set_line_wrap (text, true);
    clutter_text_set_line_wrap_mode (text, PANGO_WRAP_WORD);
    clutter_text_set_single_line_mode (text, false);
    clutter_text_set_ellipsize (text, PANGO_ELLIPSIZE_NONE);
    mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (self), priv->message,
                                             2,
                                             "x-align", MX_ALIGN_MIDDLE,
                                             NULL);
  }

  if (replace_buttons)
  {
    mx_widget_set_disabled (MX_WIDGET (priv->open), true);
    mx_widget_set_disabled (MX_WIDGET (priv->eject), true);
    clutter_container_remove_actor (CLUTTER_CONTAINER (self), priv->import);
  }

  mx_label_set_text (MX_LABEL (priv->message), message);

  if (timeout_s)
  {
    g_timeout_add_seconds (timeout_s, function, data);
  }
}


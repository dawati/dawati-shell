
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

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/statvfs.h>

#include <gio/gio.h>

#include "mpd-gobject.h"
#include "mpd-storage-device.h"
#include "config.h"

#ifdef HAVE_UDISKS
  #include <gdu/gdu.h>
#endif

static void
mpd_storage_device_set_size (MpdStorageDevice  *self,
                             uint64_t           size);

static void
mpd_storage_device_set_available_size (MpdStorageDevice  *self,
                                       uint64_t           available_size);

#define MPD_STORAGE_DEVICE_ERROR mpd_storage_device_error_quark()

static GQuark
mpd_storage_device_error_quark (void)
{
  static GQuark _quark = 0;
  if (!_quark)
    _quark = g_quark_from_static_string ("mpd-storage-device-error");
  return _quark;
}

G_DEFINE_TYPE (MpdStorageDevice, mpd_storage_device, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_STORAGE_DEVICE, MpdStorageDevicePrivate))

enum
{
  PROP_0,

  PROP_AVAILABLE_SIZE,
  PROP_LABEL,
  PROP_MODEL,
  PROP_PATH,
  PROP_SIZE,
  PROP_VENDOR
};

enum
{
  HAS_MEDIA,
  IMPORT_PROGRESS,
  IMPORT_ERROR,

  LAST_SIGNAL
};

typedef struct
{
#ifdef HAVE_UDISKS
  GduDevice     *device;
#endif
  uint64_t       available_size;
  char          *path;
  uint64_t       size;
  unsigned int   update_timeout_id;

  GSList        *dir_stack;
  GSList        *media_files;
  uint64_t       media_files_size;

  /* During import */
  GFile         *pictures_dir;
  GFile         *music_dir;
  GFile         *videos_dir;
  uint64_t       imported_size;
  GCancellable  *cancellable;
} MpdStorageDevicePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
update (MpdStorageDevice *self)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);
  struct statvfs fsd = { 0, };

  if (0 == statvfs (priv->path, &fsd))
  {
    mpd_storage_device_set_size (self, (uint64_t)
                                 fsd.f_blocks * fsd.f_frsize);
    mpd_storage_device_set_available_size (self, (uint64_t)
                                           fsd.f_bavail * fsd.f_frsize);
  } else {
    g_warning ("%s : %s", G_STRLOC, strerror (errno));
  }
}

static bool
_update_timeout_cb (MpdStorageDevice *self)
{
  update (self);
  return true;
}

#ifdef HAVE_UDISKS

static int
_find_device_cb (GduDevice  *device,
                 char const *path)
{
  char const *mount_path;

  mount_path = gdu_device_get_mount_path (device);
  return g_strcmp0 (path, mount_path);
}

GduDevice *
find_device_for_path (GList       *devices,
                      char const  *path)
{
  GList     *iter;
  GduDevice *device;

  iter = g_list_find_custom (devices, path, (GCompareFunc) _find_device_cb);

  if (iter)
  {
    device = GDU_DEVICE (iter->data);
  } else {
    char *parent = g_path_get_dirname (path);
    device = find_device_for_path (devices, parent);
    g_free (parent);
  }

  return device;
}

#endif /* HAVE_UDISKS */

static GObject *
_constructor (GType                  type,
              unsigned int           n_properties,
              GObjectConstructParam *properties)
{
  MpdStorageDevice *self = (MpdStorageDevice *)
                              G_OBJECT_CLASS (mpd_storage_device_parent_class)
                                ->constructor (type, n_properties, properties);
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);

  if (priv->path)
  {
#ifdef HAVE_UDISKS
    GduPool *pool = gdu_pool_new ();
    GList *devices = gdu_pool_get_devices (pool);
    priv->device = find_device_for_path (devices, priv->path);
    if (priv->device)
    {
      g_object_ref (priv->device);
    } else {
      /* Bail */
      g_critical ("%s : Could not find device for '%s'", G_STRLOC, priv->path);
      self = NULL;
    }
    g_list_foreach (devices, (GFunc) g_object_unref, NULL);
    g_list_free (devices);
    g_object_unref (pool);
#endif
  } else {
    /* Bail */
    self = NULL;
  }

  if (self)
  {
    update (self);
    priv->update_timeout_id =
                  g_timeout_add_seconds (60,
                                         (GSourceFunc) _update_timeout_cb,
                                         self);
  }

  return (GObject *) self;
}

static void
_get_property (GObject      *object,
               unsigned int  property_id,
               GValue       *value,
               GParamSpec   *pspec)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
  case PROP_AVAILABLE_SIZE:
    g_value_set_uint64 (value,
                        mpd_storage_device_get_available_size (
                          MPD_STORAGE_DEVICE (object)));
    break;
  case PROP_LABEL:
    g_value_set_string (value,
                        mpd_storage_device_get_label (
                          MPD_STORAGE_DEVICE (object)));
    break;
  case PROP_MODEL:
    g_value_set_string (value,
                        mpd_storage_device_get_model (
                          MPD_STORAGE_DEVICE (object)));
    break;
  case PROP_PATH:
    g_value_set_string (value, priv->path);
    break;
  case PROP_SIZE:
    g_value_set_uint64 (value,
                        mpd_storage_device_get_size (
                          MPD_STORAGE_DEVICE (object)));
    break;
  case PROP_VENDOR:
    g_value_set_string (value,
                        mpd_storage_device_get_vendor (
                          MPD_STORAGE_DEVICE (object)));
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
  MpdStorageDevicePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
  case PROP_AVAILABLE_SIZE:
    mpd_storage_device_set_available_size (MPD_STORAGE_DEVICE (object),
                                           g_value_get_uint64 (value));
    break;
  case PROP_PATH:
    /* Construct-only */
    priv->path = g_value_dup_string (value);
    break;
  case PROP_SIZE:
    mpd_storage_device_set_size (MPD_STORAGE_DEVICE (object),
                                 g_value_get_uint64 (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (object);

#ifdef HAVE_UDISKS
  mpd_gobject_detach (object, (GObject **) &priv->device);
#endif

  if (priv->path)
  {
    g_free (priv->path);
    priv->path = NULL;
  }

  if (priv->update_timeout_id)
  {
    g_source_remove (priv->update_timeout_id);
    priv->update_timeout_id = 0;
  }

  if (priv->media_files)
  {
    g_slist_foreach (priv->media_files, (GFunc) g_free, NULL);
    g_slist_free (priv->media_files);
    priv->media_files = NULL;
  }

  if (priv->pictures_dir)
  {
    g_object_unref (priv->pictures_dir);
    priv->pictures_dir = NULL;
  }

  if (priv->music_dir)
  {
    g_object_unref (priv->music_dir);
    priv->music_dir = NULL;
  }

  if (priv->videos_dir)
  {
    g_object_unref (priv->videos_dir);
    priv->videos_dir = NULL;
  }

  if (priv->cancellable)
  {
    g_object_unref (priv->cancellable);
    priv->cancellable = NULL;
  }

  G_OBJECT_CLASS (mpd_storage_device_parent_class)->dispose (object);
}

static void
mpd_storage_device_class_init (MpdStorageDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdStorageDevicePrivate));

  object_class->constructor = _constructor;
  object_class->dispose = _dispose;
  object_class->get_property = _get_property;
  object_class->set_property = _set_property;

  /* Properties */

  param_flags = G_PARAM_READABLE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_AVAILABLE_SIZE,
                                   g_param_spec_uint64 ("available-size",
                                                        "Available size",
                                                        "Available disk size",
                                                        0, G_MAXUINT64, 0,
                                                        param_flags));
  g_object_class_install_property (object_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        "Label",
                                                        "Storage device label",
                                                        NULL,
                                                        param_flags));
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_string ("model",
                                                        "Model",
                                                        "Storage device model",
                                                        NULL,
                                                        param_flags));
  g_object_class_install_property (object_class,
                                   PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "Path",
                                                        "Mount path",
                                                        NULL,
                                                        param_flags |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_WRITABLE));
  g_object_class_install_property (object_class,
                                   PROP_SIZE,
                                   g_param_spec_uint64 ("size",
                                                        "Size",
                                                        "Disk size",
                                                        0, G_MAXUINT64, 0,
                                                        param_flags));
  g_object_class_install_property (object_class,
                                   PROP_VENDOR,
                                   g_param_spec_string ("vendor",
                                                        "Vendor",
                                                        "Storage device vendor",
                                                        NULL,
                                                        param_flags));

  /* Signals */

  _signals[HAS_MEDIA] = g_signal_new ("has-media",
                                      G_TYPE_FROM_CLASS (klass),
                                      G_SIGNAL_RUN_LAST,
                                      0, NULL, NULL,
                                      g_cclosure_marshal_VOID__BOOLEAN,
                                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  _signals[IMPORT_PROGRESS] = g_signal_new ("import-progress",
                                            G_TYPE_FROM_CLASS (klass),
                                            G_SIGNAL_RUN_LAST,
                                            0, NULL, NULL,
                                            g_cclosure_marshal_VOID__FLOAT,
                                            G_TYPE_NONE, 1, G_TYPE_FLOAT);

  _signals[IMPORT_ERROR] = g_signal_new ("import-error",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__POINTER,
                                         G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
mpd_storage_device_init (MpdStorageDevice *self)
{
}

MpdStorageDevice *
mpd_storage_device_new (char const *path)
{
  return g_object_new (MPD_TYPE_STORAGE_DEVICE,
                       "path", path,
                       NULL);
}

uint64_t
mpd_storage_device_get_size (MpdStorageDevice *self)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE (self), 0);

  return priv->size;
}

static void
mpd_storage_device_set_size (MpdStorageDevice  *self,
                          uint64_t           size)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_STORAGE_DEVICE (self));

  if (size != priv->size)
  {
    priv->size = size;
    g_object_notify (G_OBJECT (self), "size");
  }
}

uint64_t
mpd_storage_device_get_available_size (MpdStorageDevice *self)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE (self), 0);

  return priv->available_size;
}

static void
mpd_storage_device_set_available_size (MpdStorageDevice  *self,
                                       uint64_t           available_size)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_STORAGE_DEVICE (self));

  if (available_size != priv->available_size)
  {
    priv->available_size = available_size;
    g_object_notify (G_OBJECT (self), "available-size");
  }
}

char const *
mpd_storage_device_get_label (MpdStorageDevice *self)
{
#ifdef HAVE_UDISKS
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE (self), NULL);

  return gdu_device_get_presentation_name (priv->device);
#else
  return NULL;
#endif
}

char const *
mpd_storage_device_get_model (MpdStorageDevice *self)
{
#ifdef HAVE_UDISKS
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE (self), NULL);

  return gdu_device_drive_get_model (priv->device);
#else
  return NULL;
#endif
}

char const *
mpd_storage_device_get_path (MpdStorageDevice *self)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE (self), NULL);

  return priv->path;
}

char const *
mpd_storage_device_get_vendor (MpdStorageDevice *self)
{
#ifdef HAVE_UDISKS
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);

  return gdu_device_drive_get_vendor (priv->device);
#else
  return NULL;
#endif
}

static void
push_dir_async (MpdStorageDevice  *self,
                GFile             *dir);

static GFile *
file_new_for_path (char const *path)
{
  g_debug ("%s() %s", __FUNCTION__, path);
  return g_file_new_for_path (path);
}

static void
file_destroy (GFile *file)
{
  char *path;

  path = g_file_get_path (file);
  g_debug ("%s() %s (%d)", __FUNCTION__, path,  G_OBJECT (file)->ref_count);
  g_free (path);

  g_object_unref (file);
}

static void
enumerate_dir (MpdStorageDevice  *self)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);
  GFileEnumerator *enumerator;
  GFileInfo       *info;
  GError          *error = NULL;

  g_return_if_fail (priv->dir_stack);
  enumerator = G_FILE_ENUMERATOR (priv->dir_stack->data);
  g_return_if_fail (G_IS_FILE_ENUMERATOR (enumerator));

  while (NULL !=
         (info = g_file_enumerator_next_file (enumerator, NULL, &error)))
  {
    char const *content_type = g_file_info_get_content_type (info);

    /* Debug */
    char *indent = g_strnfill (g_slist_length (priv->dir_stack), ' ');
    g_debug ("%s%s %s", indent, g_file_info_get_name (info), content_type);
    g_free (indent);

    if (0 == g_strcmp0 ("inode/directory", content_type))
    {
      char const *path = g_object_get_data (G_OBJECT (enumerator), "path");
      char *subpath = g_build_filename (path, g_file_info_get_name (info), NULL);
      GFile *subdir = file_new_for_path (subpath);

      /* Push and recurse. */
      push_dir_async (self, subdir);

      file_destroy (subdir);
      g_free (subpath);
      break;

    } else if (g_str_has_prefix (content_type, "audio/") ||
               g_str_has_prefix (content_type, "image/") ||
               g_str_has_prefix (content_type, "video/")) {

      /* Media found. */
      char const *path = g_object_get_data (G_OBJECT (enumerator), "path");
      char *filename = g_build_filename (path, g_file_info_get_name (info), NULL);
      priv->media_files = g_slist_prepend (priv->media_files, filename);
      priv->media_files_size += g_file_info_get_size (info);
    }

    g_object_unref (info);
  }

  if (info)
  {
    /* Broke out of loop, not done yet. */
    g_object_unref (info);

  } else {

    /* Directory finished, pop. */
    g_object_unref (enumerator);
    priv->dir_stack = g_slist_delete_link (priv->dir_stack, priv->dir_stack);
    if (priv->dir_stack)
    {
      enumerate_dir (self);
    } else {
      /* Done iterating. */
      g_signal_emit_by_name (self, "has-media", (bool) priv->media_files);
    }
  }

  if (error)
  {
    g_critical ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
}

static void
_push_dir_cb (GFile             *file,
              GAsyncResult      *res,
              MpdStorageDevice  *self)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);
  GFileEnumerator *enumerator;
  GError          *error = NULL;

  g_return_if_fail (G_IS_FILE (file));

  /* Debug */
  char *indent = g_strnfill (g_slist_length (priv->dir_stack), ' ');
  char *path = g_file_get_path (file);
  g_debug ("%s%s", indent, path);
  g_free (path);
  g_free (indent);

  enumerator = g_file_enumerate_children_finish (file, res, &error);
  if (error)
  {
    g_critical ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
    return;
  }

  /* g_file_enumerator_get_container () fails on us. */
  g_object_set_data_full (G_OBJECT (enumerator), "path",
                          g_file_get_path (file), g_free);
  priv->dir_stack = g_slist_prepend (priv->dir_stack,
                                     g_object_ref (enumerator));
  enumerate_dir (self);
  g_object_unref (enumerator);
}

static void
push_dir_async (MpdStorageDevice  *self,
                GFile             *dir)
{
  char *path;
  bool  is_dir;

  path = g_file_get_path (dir);
  is_dir = g_file_test (path, G_FILE_TEST_IS_DIR);
  g_free (path);
  g_return_if_fail (is_dir);

  g_file_enumerate_children_async (dir, "standard", G_FILE_QUERY_INFO_NONE,
                                   G_PRIORITY_DEFAULT, NULL,
                                   (GAsyncReadyCallback) _push_dir_cb,
                                   self);
}

void
mpd_storage_device_has_media_async (MpdStorageDevice *self)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);
  GFile *dir;

  g_return_if_fail (MPD_IS_STORAGE_DEVICE (self));

  dir = file_new_for_path (priv->path);
  push_dir_async (self, dir);
  file_destroy (dir);
}

static GFile *
ensure_import_subdir (char const   *path,
                      GError      **error)
{
  GTimeVal   tv = { 0, };
  GDate      date = { 0, };
  char       template[PATH_MAX] = { 0, } /* whatever */;
  GFile     *basedir;
  GFile     *subdir;
  unsigned int  i = 0;

  g_return_val_if_fail (g_file_test (path, G_FILE_TEST_IS_DIR), NULL);

  g_get_current_time (&tv);
  g_date_set_time_val (&date, &tv);

  basedir = g_file_new_for_path (path);
  g_date_strftime (template, sizeof (template), "%x", &date);
  /* We like locale specific dir names but '/' don't work very well.
   * so fall back to ISO date format. */
  if (strchr (template, '/'))
  {
    memset (template, 0, sizeof (template));
    g_date_strftime (template, sizeof (template), "%Y-%m-%d", &date);
  }

  subdir = g_file_get_child (basedir, template);
  while (g_file_query_exists (subdir, NULL))
  {
    char *subdir_name = g_strdup_printf ("%s (%d)", template, ++i);
    g_object_unref (subdir);
    subdir = g_file_get_child (basedir, subdir_name);
    g_free (subdir_name);
  }

  if (!g_file_make_directory (subdir, NULL, error))
  {
    g_object_unref (subdir);
    subdir = NULL;
  }

  g_object_unref (basedir);
  return subdir;
}

static void
import_file_async (MpdStorageDevice *self);

static void
_file_copy_cb (GFile            *source_file,
               GAsyncResult     *res,
               MpdStorageDevice *self)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);
  GFileInfo *info;
  GError    *error = NULL;
  float      progress;

  g_file_copy_finish (source_file, res, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_signal_emit_by_name (self, "import-error", error);
    g_clear_error (&error);
    return;
  }

  info = g_file_query_info (source_file, "standard", G_FILE_QUERY_INFO_NONE,
                            NULL, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_signal_emit_by_name (self, "import-error", error);
    g_clear_error (&error);
    return;
  }

  priv->imported_size += g_file_info_get_size (info);
  progress = (float) priv->imported_size / priv->media_files_size;
  g_signal_emit_by_name (self, "import-progress", progress);
  g_object_unref (info);

  g_debug ("%s() %f", __FUNCTION__, progress);

  /* Next file. */
  if (priv->media_files)
    import_file_async (self);
}

static void
import_file_async (MpdStorageDevice *self)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);
  char        *source_path = NULL;
  GFile       *source_file = NULL;
  GFileInfo   *source_info = NULL;
  char const  *content_type;
  GFile       *target_dir = NULL;
  char        *target_name = NULL;
  GFile       *target_file = NULL;
  GError      *error = NULL;

  g_return_if_fail (priv->media_files);

  source_path = (char *) priv->media_files->data;
  priv->media_files = g_slist_delete_link (priv->media_files, priv->media_files);

  source_file = g_file_new_for_path (source_path);
  source_info = g_file_query_info (source_file, "standard",
                                   G_FILE_QUERY_INFO_NONE, NULL, &error);
  if (error)
  {
    g_signal_emit_by_name (self, "import-error", error);
    g_clear_error (&error);
    goto bail;
  }

  content_type = g_file_info_get_content_type (source_info);
  if (g_str_has_prefix (content_type, "audio/"))
  {
    if (NULL == priv->music_dir)
    {
      priv->music_dir =
        ensure_import_subdir (g_get_user_special_dir(G_USER_DIRECTORY_MUSIC),
                              &error);
      if (error)
      {
        g_signal_emit_by_name (self, "import-error", error);
        g_clear_error (&error);
        goto bail;
      }
    }
    target_dir = priv->music_dir;

  } else if (g_str_has_prefix (content_type, "image/")) {

    if (NULL == priv->pictures_dir)
    {
      priv->pictures_dir =
        ensure_import_subdir (g_get_user_special_dir (G_USER_DIRECTORY_PICTURES),
                              &error);
      if (error)
      {
        g_signal_emit_by_name (self, "import-error", error);
        g_clear_error (&error);
        goto bail;
      }
    }
    target_dir = priv->pictures_dir;

  } else if (g_str_has_prefix (content_type, "video/")) {

    if (NULL == priv->pictures_dir)
    {
      priv->videos_dir =
        ensure_import_subdir (g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS),
                              &error);
      if (error)
      {
        g_signal_emit_by_name (self, "import-error", error);
        g_clear_error (&error);
        goto bail;
      }
    }
    target_dir = priv->videos_dir;

  } else {

    g_warning ("%s : Unhandled content type %s", G_STRLOC, content_type);
    goto bail;
  }

  target_name = g_path_get_basename (source_path);
  target_file = g_file_get_child (target_dir, target_name);
  g_file_copy_async (source_file, target_file, G_FILE_COPY_NONE,
                     G_PRIORITY_DEFAULT, priv->cancellable,
                     NULL, NULL,
                     (GAsyncReadyCallback) _file_copy_cb, self);

bail:
  if (target_file) g_object_unref (target_file);
  if (target_name) g_free (target_name);
  if (source_info) g_object_unref (source_info);
  if (source_file) g_object_unref (source_file);
  if (source_path) g_free (source_path);
}

bool
mpd_storage_device_import_async (MpdStorageDevice  *self,
                                 GError           **error)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);
  MpdStorageDevice *target;
  uint64_t          target_available;

  g_return_val_if_fail (MPD_IS_STORAGE_DEVICE (self), false);

  if (priv->dir_stack)
  {
    g_warning ("%s : %s: Device indexing in progress",
                G_STRLOC,
                priv->path);
    if (error)
      *error = g_error_new (MPD_STORAGE_DEVICE_ERROR,
                            MPD_STORAGE_DEVICE_IMPORT_ERROR_STILL_INDEXING,
                            "%s : %s: Device indexing in progress",
                            G_STRLOC, priv->path);
    return false;
  }

  if (NULL == priv->media_files)
  {
    g_warning ("%s : %s: No media to import",
                G_STRLOC,
                priv->path);
    if (error)
      *error = g_error_new (MPD_STORAGE_DEVICE_ERROR,
                            MPD_STORAGE_DEVICE_IMPORT_ERROR_NO_MEDIA,
                            "%s : %s: No media to import",
                            G_STRLOC, priv->path);
    return false;
  }

  target = mpd_storage_device_new (g_get_home_dir ());
  target_available = mpd_storage_device_get_available_size (target);
  g_object_unref (target);
  if (target_available < priv->media_files_size)
  {
    char *available_text = g_format_size_for_display (target_available);
    char *required_text = g_format_size_for_display (priv->media_files_size);
    g_warning ("%s : Would need %s on %s but only %s available",
               G_STRLOC,
               available_text,
               g_get_home_dir (),
               required_text);
    if (error)
      *error = g_error_new (MPD_STORAGE_DEVICE_ERROR,
                            MPD_STORAGE_DEVICE_IMPORT_ERROR_INSUFICCIENT_DISK_SPACE,
                            "%s : Would need %s on %s but only %s available",
                            G_STRLOC,
                            available_text,
                            g_get_home_dir (),
                            required_text);
    g_free (available_text);
    g_free (required_text);
    return false;
  }

  priv->cancellable = g_cancellable_new ();
  import_file_async (self);
  return true;
}

bool
mpd_storage_device_stop_import (MpdStorageDevice *self)
{
  MpdStorageDevicePrivate *priv = GET_PRIVATE (self);
  g_cancellable_cancel (priv->cancellable);
  return true;
}


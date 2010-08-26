
/*
 * Copyright (c) 2010 Intel Corporation.
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
 *
 * Author: Rob Staudinger <robsta@linux.intel.com>
 */

/*
 * FIXME
 * Maybe create an "mpl-app-launches-store-insert" executable that
 * can be spawned from any process that wants to insert. This would
 * delegate the queueing to the kernel.
 */

#define _GNU_SOURCE /* for comparison_fn_t from stdlib.h */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <gio/gio.h>

#include <meego-panel/mpl-app-launches-store-priv.h>

G_DEFINE_TYPE (MplAppLaunchesStore, mpl_app_launches_store, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_APP_LAUNCHES_STORE, MplAppLaunchesStorePrivate))

enum
{
  PROP_0,
  PROP_DATABASE_FILE
};

enum
{
  CHANGED,

  LAST_SIGNAL
};

/*
 * Record as persisted to disk.
 */
typedef struct
{
  char    hash[9];          /* Hash of executable name as hex-string, incl. '\n' */
  char    last_launched[9]; /* Hash of time_t as hex-string, incl. '\n' */
  char    n_launches[9];    /* Hash of total launches as hex string, incl. '\n' */
  char    newline;
} MplAppLaunchesRecord;

typedef struct
{
  char                  *database_file;
  GFileMonitor          *monitor;
  int                    fd;
  MplAppLaunchesRecord  *data;
  size_t                 size;
  unsigned               mmap_reference_count;
  bool                   for_writing;
} MplAppLaunchesStorePrivate;

#define PROPAGATE_ERROR_AND_RETURN_IF_FAIL(condition_, error_, error_ptr_)  \
          if (!(condition_))                                                \
          {                                                                 \
            g_critical ("%s\n\t%s", G_STRLOC, error_->message);             \
            if (error_ptr_)                                                 \
              *error_ptr_ = error_;                                         \
            else                                                            \
              g_clear_error (&error_);                                      \
            return false;                                                   \
          }

#define PROPAGATE_ERROR_AND_RETURN_IF_FAIL_WITH_CODE(condition_, error_, error_ptr_, code_)  \
          if (!(condition_))                                                \
          {                                                                 \
            code_;                                                          \
            g_critical ("%s\n\t%s", G_STRLOC, error_->message);             \
            if (error_ptr_)                                                 \
              *error_ptr_ = error_;                                         \
            else                                                            \
              g_clear_error (&error_);                                      \
            return false;                                                   \
          }

#define MPL_APP_LAUNCHES_STORE_ERROR mpl_app_launches_store_error_quark()

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static GQuark
mpl_app_launches_store_error_quark (void)
{
  static GQuark _quark = 0;
  if (!_quark)
    _quark = g_quark_from_static_string ("mpl-app-launches-store-error");
  return _quark;
}

static void
_database_file_changed_cb (GFileMonitor        *monitor,
                           GFile               *file,
                           GFile               *other_file,
                           GFileMonitorEvent    event_type,
                           MplAppLaunchesStore *self)
{
  g_signal_emit_by_name (self, "changed");
}

static void
_get_property (GObject    *object,
               unsigned    property_id,
               GValue     *value,
               GParamSpec *pspec)
{
  MplAppLaunchesStorePrivate *priv = GET_PRIVATE (object);

  switch (property_id)
  {
  case PROP_DATABASE_FILE:
    g_value_set_string (value, priv->database_file);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               unsigned      property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  MplAppLaunchesStorePrivate *priv = GET_PRIVATE (object);

  switch (property_id)
  {
  case PROP_DATABASE_FILE:
    /* Construct-only */
    priv->database_file = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_constructed (GObject *object)
{
  MplAppLaunchesStorePrivate *priv = GET_PRIVATE (object);
  GFile   *file;
  GError  *error = NULL;

  file = g_file_new_for_path (priv->database_file);
  priv->monitor = g_file_monitor (file, G_FILE_MONITOR_NONE, NULL, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
  } else {
    g_signal_connect (priv->monitor, "changed",
                      G_CALLBACK (_database_file_changed_cb), object);
  }
}

static void
_dispose (GObject *object)
{
  MplAppLaunchesStorePrivate *priv = GET_PRIVATE (object);

  if (priv->database_file)
  {
    g_free (priv->database_file);
    priv->database_file = NULL;
  }

  if (priv->monitor)
  {
    g_object_unref (priv->monitor);
    priv->monitor = NULL;
  }

  G_OBJECT_CLASS (mpl_app_launches_store_parent_class)->dispose (object);
}

static void
mpl_app_launches_store_class_init (MplAppLaunchesStoreClass *klass)
{
  static char   *_database_file = NULL;
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GParamFlags    param_flags;

  g_type_class_add_private (klass, sizeof (MplAppLaunchesStorePrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->constructed = _constructed;
  object_class->dispose = _dispose;

  /* Properties */

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  if (NULL == _database_file)
  {
    _database_file = g_build_filename (g_get_user_cache_dir (),
                                       "meego-app-launches", NULL);
  }

  g_object_class_install_property (object_class,
                                   PROP_DATABASE_FILE,
                                   g_param_spec_string ("database-file",
                                                        "Database file",
                                                        "File where app launches are logged",
                                                        _database_file,
                                                        param_flags |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /* Signals */

  _signals[CHANGED] = g_signal_new ("changed",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    0, NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);
}

static void
mpl_app_launches_store_init (MplAppLaunchesStore *self)
{
}

MplAppLaunchesStore *
mpl_app_launches_store_new (void)
{
  return g_object_new (MPL_TYPE_APP_LAUNCHES_STORE, NULL);
}

static int
_compare_cb (MplAppLaunchesRecord const *a,
             MplAppLaunchesRecord const *b)
{
  int ret;

  /* strncmp ought to work for padded '\0' terminated hex strings  */
  ret = strncmp (a->hash, b->hash, sizeof (a->hash) - 1);
  return ret;
}

static void
record_read (MplAppLaunchesRecord const *record,
             uint32_t                   *hash_out,
             time_t                     *last_launched_out,
             uint32_t                   *n_launches_out)
{
  if (hash_out)
    sscanf (record->hash, "%x", hash_out);

  if (last_launched_out)
    sscanf (record->last_launched, "%lx", last_launched_out);

  if (n_launches_out)
    sscanf (record->n_launches, "%x", n_launches_out);
}

static void
record_update (MplAppLaunchesRecord *record,
               int32_t               hash,
               time_t                timestamp)
{
  uint32_t n_launches = 0;

  if (hash)
    snprintf (record->hash, sizeof (record->hash), "%08x", hash);

  if (timestamp)
    snprintf (record->last_launched, sizeof (record->last_launched),
              "%08lx", timestamp);

  record_read (record, NULL, NULL, &n_launches);
  snprintf (record->n_launches, sizeof (record->n_launches),
            "%08x", n_launches + 1);
}

static bool
record_write (MplAppLaunchesRecord   *record,
              int                     fd,
              GError                **error_out)
{
  ssize_t n_bytes;

  n_bytes = write (fd, record, sizeof (*record));
  if (n_bytes < 0)
  {
    if (error_out)
      *error_out = g_error_new (MPL_APP_LAUNCHES_STORE_ERROR,
                                MPL_APP_LAUNCHES_STORE_ERROR_WRITING_DATABASE,
                                "%s : %s",
                                G_STRLOC, strerror (errno));
    return false;
  }

  return true;
}

/*
 * Open the store for reading or writing.
 * This is private API, the one-shot functions handle opening and closing
 * the store.
 */
bool
mpl_app_launches_store_open (MplAppLaunchesStore  *self,
                             bool                  for_writing,
                             GError              **error_out)
{
  MplAppLaunchesStorePrivate *priv = GET_PRIVATE (self);
  int         open_mode;
  int         mmap_protect;
  int         lock_flags;
  struct stat sb;
  bool        ret = true;

  if (priv->mmap_reference_count > 0)
  {
    /* Already mapped. */
    if (priv->for_writing)
    {
      if (error_out)
        *error_out = g_error_new (MPL_APP_LAUNCHES_STORE_ERROR,
                                  MPL_APP_LAUNCHES_STORE_ERROR_OPENING_DATABASE,
                                  "Database opened for writing and locked");
      return false;

    } else {

      priv->mmap_reference_count++;
      return true;
    }
  }

  priv->data = NULL;
  priv->size = 0;
  priv->for_writing = for_writing;

  /* Empty (non existant) store is fine. */
  if (!g_file_test (priv->database_file, G_FILE_TEST_IS_REGULAR))
  {
    return true;
  }

  if (priv->for_writing)
  {
    open_mode = O_RDWR;
    mmap_protect = PROT_READ | PROT_WRITE;
    lock_flags = LOCK_EX;
  } else {
    open_mode = O_RDONLY;
    mmap_protect = PROT_READ;
    lock_flags = LOCK_SH;
  }

  priv->fd = open (priv->database_file, open_mode);
  if (-1 == priv->fd)
  {
    if (error_out)
      *error_out = g_error_new (MPL_APP_LAUNCHES_STORE_ERROR,
                                MPL_APP_LAUNCHES_STORE_ERROR_OPENING_DATABASE,
                                "%s : %s",
                                G_STRLOC, strerror (errno));
    return false;
  }

  if (-1 == flock (priv->fd, lock_flags))
  {
    if (error_out)
      *error_out = g_error_new (MPL_APP_LAUNCHES_STORE_ERROR,
                                MPL_APP_LAUNCHES_STORE_ERROR_OPENING_DATABASE,
                                "%s : %s",
                                G_STRLOC, strerror (errno));
    return false;
  }

  if (-1 == fstat (priv->fd, &sb))
  {
    if (error_out)
      *error_out = g_error_new (MPL_APP_LAUNCHES_STORE_ERROR,
                                MPL_APP_LAUNCHES_STORE_ERROR_READING_DATABASE,
                                "%s : %s",
                                G_STRLOC, strerror (errno));
    return false;
  }

  priv->size = sb.st_size;
  priv->data = mmap (0, sb.st_size, mmap_protect, MAP_SHARED, priv->fd, 0);
  if ((void *) -1 == priv->data)
  {
    if (error_out)
      *error_out = g_error_new (MPL_APP_LAUNCHES_STORE_ERROR,
                                MPL_APP_LAUNCHES_STORE_ERROR_OPENING_DATABASE,
                                "%s : %s",
                                G_STRLOC, strerror (errno));
    priv->fd = 0;
    priv->data = NULL;
    priv->size = 0;
    ret = false;
  } else {
    priv->mmap_reference_count = 1;
  }

  return ret;
}

/*
 * Close the store after reading or writing.
 * This is private API, the one-shot functions handle opening and closing
 * the store.
 */
bool
mpl_app_launches_store_close (MplAppLaunchesStore  *self,
                              GError              **error_out)
{
  MplAppLaunchesStorePrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;

  if (priv->mmap_reference_count > 1)
  {
    priv->mmap_reference_count--;
    return true;
  }

  if (priv->data && priv->size)
  {
    /* Store is not empty/non-exist, close it. */

    if (-1 == munmap ((void *) priv->data, priv->size))
    {
      error = g_error_new (MPL_APP_LAUNCHES_STORE_ERROR,
                           MPL_APP_LAUNCHES_STORE_ERROR_CLOSING_DATABASE,
                           "%s : %s",
                           G_STRLOC, strerror (errno));
    }

    if (-1 == flock (priv->fd, LOCK_UN))
    {
      g_warning ("%s : %s", G_STRLOC, strerror (errno));
    }

    if (-1 == close (priv->fd))
    {
      g_warning ("%s : %s", G_STRLOC, strerror (errno));
    }

    priv->fd = 0;
    priv->data = NULL;
    priv->size = 0;
    priv->mmap_reference_count = 0;

    PROPAGATE_ERROR_AND_RETURN_IF_FAIL (!error, error, error_out);
  }

  return true;
}

static MplAppLaunchesRecord *
store_lookup_hash (MplAppLaunchesStore  *self,
                   uint32_t              hash,
                   GError              **error_out)
{
  MplAppLaunchesStorePrivate *priv = GET_PRIVATE (self);
  MplAppLaunchesRecord  *record;
  MplAppLaunchesRecord   key;

  snprintf (key.hash, sizeof (key.hash), "%08x", hash);

  record = bsearch (&key, priv->data,
                    priv->size / sizeof (MplAppLaunchesRecord),
                    sizeof (MplAppLaunchesRecord),
                    (comparison_fn_t) _compare_cb);

  return record;
}

static char *
store_insert (MplAppLaunchesStore   *self,
              uint32_t               hash,
              time_t                 timestamp,
              GError               **error_out)
{
  MplAppLaunchesStorePrivate *priv = GET_PRIVATE (self);
  char       template[] = "/tmp/meego-app-launches-store-XXXXXX";
  int        fd;
  unsigned   n_elements;
  unsigned   i;
  GError    *error = NULL;

  fd = mkstemp (template);
  if (-1 == fd)
  {
    if (error_out)
      *error_out = g_error_new (MPL_APP_LAUNCHES_STORE_ERROR,
                                MPL_APP_LAUNCHES_STORE_ERROR_OPENING_DATABASE,
                                "%s : %s",
                                G_STRLOC, strerror (errno));
    return NULL;
  }

  n_elements = priv->size / sizeof (MplAppLaunchesRecord);
  for (i = 0; i < n_elements; i++)
  {
    if (hash)
    {
      /* Test for insert position. */
      uint32_t current_hash;
      record_read (&priv->data[i], &current_hash, NULL, NULL);
      if (current_hash > hash)
      {
        /* Insert new record here. */
        MplAppLaunchesRecord record = { "00000000", "00000000", "00000000", '\n' };
        record_update (&record, hash, timestamp);
        hash = 0;
        if (!record_write (&record, fd, &error))
          break;
      }
    }

    if (!record_write (&priv->data[i], fd, &error))
      break;
  }

  if (hash)
  {
    /* Insert new record at the end. */
    MplAppLaunchesRecord record = { "00000000", "00000000", "00000000", '\n' };
    record_update (&record, hash, timestamp);
    record_write (&record, fd, &error);
  }

  PROPAGATE_ERROR_AND_RETURN_IF_FAIL_WITH_CODE (!error, error, error_out, close (fd));

  if (-1 == close (fd))
  {
    if (error_out)
      *error_out = g_error_new (MPL_APP_LAUNCHES_STORE_ERROR,
                                MPL_APP_LAUNCHES_STORE_ERROR_CLOSING_DATABASE,
                                "%s : %s",
                                G_STRLOC, strerror (errno));
    return NULL;
  }

  return g_strdup (template);
}

/*
 * Add executable launch event to the store.
 * When 0 is passed for timestamp the current time is used.
 */
bool
mpl_app_launches_store_add (MplAppLaunchesStore  *self,
                            char const           *executable,
                            time_t                timestamp,
                            GError              **error_out)
{
  MplAppLaunchesStorePrivate *priv = GET_PRIVATE (self);
  MplAppLaunchesRecord  *record = NULL;
  uint32_t               hash;
  GError                *error = NULL;

  g_return_val_if_fail (MPL_IS_APP_LAUNCHES_STORE (self), false);

  mpl_app_launches_store_open (self, true, &error);
  PROPAGATE_ERROR_AND_RETURN_IF_FAIL (!error, error, error_out);

  hash = g_str_hash (executable);

  /* Already got a record for this executable? */
  record = store_lookup_hash (self, hash, &error);
  PROPAGATE_ERROR_AND_RETURN_IF_FAIL (!error, error, error_out);

  if (record)
  {
    record_update (record, 0, timestamp ? timestamp : time (NULL));

  } else {

    char *tmp_database_file = store_insert (self,
                                            hash,
                                            timestamp ? timestamp : time (NULL),
                                            &error);
    PROPAGATE_ERROR_AND_RETURN_IF_FAIL_WITH_CODE (tmp_database_file,
                                                  error,
                                                  error_out,
                                                  mpl_app_launches_store_close (self, NULL));

    if (-1 == rename (tmp_database_file, priv->database_file))
    {
      error = g_error_new (MPL_APP_LAUNCHES_STORE_ERROR,
                           MPL_APP_LAUNCHES_STORE_ERROR_WRITING_DATABASE,
                           "%s : %s",
                           G_STRLOC, strerror (errno));
    }

    g_free (tmp_database_file);
  }

  mpl_app_launches_store_close (self, &error);
  PROPAGATE_ERROR_AND_RETURN_IF_FAIL (!error, error, error_out);

  return true;
}

bool
mpl_app_launches_store_add_async (MplAppLaunchesStore  *self,
                                  char const           *executable,
                                  time_t                timestamp,
                                  GError              **error)
{
  char  *command_line;
  bool   ret;

  command_line = g_strdup_printf ("%s --add %s --timestamp %li",
                                  MEEGO_APP_LAUNCHES_STORE,
                                  executable,
                                  timestamp ? timestamp : time (NULL));

  ret = g_spawn_command_line_async (command_line, error);
  g_free (command_line);

  return ret;
}

/*
 * Look up executable.
 * For looking up a number of executables please refer to
 * MplAppLaunchesQuery, as it is more efficient for subsequent lookups.
 */
bool
mpl_app_launches_store_lookup (MplAppLaunchesStore   *self,
                               char const            *executable,
                               time_t                *last_launched_out,
                               uint32_t              *n_launches_out,
                               GError               **error_out)
{
  MplAppLaunchesRecord  *record;
  uint32_t               hash;
  GError                *error = NULL;

  g_return_val_if_fail (MPL_IS_APP_LAUNCHES_STORE (self), false);

  mpl_app_launches_store_open (self, false, &error);
  PROPAGATE_ERROR_AND_RETURN_IF_FAIL (!error, error, error_out);

  hash = g_str_hash (executable);
  record = store_lookup_hash (self, hash, &error);
  PROPAGATE_ERROR_AND_RETURN_IF_FAIL_WITH_CODE (!error, error, error_out,
                                                mpl_app_launches_store_close (self, NULL));

  if (record)
  {
    record_read (record, NULL, last_launched_out, n_launches_out);
  }

  mpl_app_launches_store_close (self, &error);
  PROPAGATE_ERROR_AND_RETURN_IF_FAIL (!error, error, error_out);

  return (bool) record;
}

/*
 * Dump the store to stdout.
 */
bool
mpl_app_launches_store_dump (MplAppLaunchesStore   *self,
                             GError               **error_out)
{
  MplAppLaunchesStorePrivate *priv = GET_PRIVATE (self);
  unsigned               n_elements;
  unsigned               i;
  GError                *error = NULL;

  g_return_val_if_fail (MPL_IS_APP_LAUNCHES_STORE (self), false);

  mpl_app_launches_store_open (self, false, &error);
  PROPAGATE_ERROR_AND_RETURN_IF_FAIL (!error, error, error_out);

  n_elements = priv->size / sizeof (MplAppLaunchesRecord);
  for (i = 0; i < n_elements; i++)
  {
    uint32_t hash;
    time_t  last_launched;
    uint32_t n_launches;
    struct tm last_launched_tm;
    char last_launched_str[64] = { 0, };

    record_read (&priv->data[i], &hash, &last_launched, &n_launches);

    localtime_r (&last_launched, &last_launched_tm);
    strftime (last_launched_str, sizeof (last_launched_str),
              "%Y-%m-%d %H:%M:%S", &last_launched_tm);
    printf ("%08x\t%s\t%i\n", hash, last_launched_str, n_launches);
  }

  mpl_app_launches_store_close (self, &error);
  PROPAGATE_ERROR_AND_RETURN_IF_FAIL (!error, error, error_out);

  return true;
}

/*
 * Create a query object for efficiently looking up multiple executables.
 * The object needs to be unref'd after being used.
 */
MplAppLaunchesQuery *
mpl_app_launches_store_create_query (MplAppLaunchesStore *self)
{
  return g_object_new (MPL_TYPE_APP_LAUNCHES_QUERY,
                       "store", self,
                       NULL);
}


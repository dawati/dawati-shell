/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib-object.h>
#include <glib/gi18n.h>
#include "mnb-launcher-entry.h"

/*
 * Desktop file utils.
 */

static gchar *
desktop_file_get_name (GKeyFile *entry)
{
  gchar *ret = NULL;
  GError *error = NULL;

  g_return_val_if_fail (entry, NULL);

  ret = g_key_file_get_locale_string (entry,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_GENERIC_NAME,
                                      NULL,
                                      &error);
  if (error)
    {
      /* Missing generic name is very common, so just ignore.
       * g_warning ("%s", error->message); */
      g_clear_error (&error);
    }

  if (ret)
    return ret;

  /* Fall back to "Name" */
  ret = g_key_file_get_locale_string (entry,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_NAME,
                                      NULL,
                                      &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_clear_error (&error);
      return NULL;
    }

  return ret;
}

/*
 * Get executable from menu entry and check it's available in the path.
 * Returns: absolute path if found, otherwise NULL.
 */
static gchar *
desktop_file_get_exec (GKeyFile *entry)
{
  gchar   *exec = NULL;
  gchar   *binary = NULL;
  gint     argc;
  gchar  **argv;
  GError  *error = NULL;

  g_return_val_if_fail (entry, NULL);

  error = NULL;
  exec = g_key_file_get_value (entry,
                               G_KEY_FILE_DESKTOP_GROUP,
                               G_KEY_FILE_DESKTOP_KEY_EXEC, &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  if (exec)
    {
      error = NULL;
      if (g_shell_parse_argv (exec, &argc, &argv, &error))
        {
          binary = g_find_program_in_path (argv[0]);
          g_strfreev (argv);
        }
      else
        {
          g_warning ("%s", error->message);
          g_error_free (error);
        }
      g_free (exec);
    }

  return binary;
}

static gchar *
desktop_file_get_icon (GKeyFile *entry)
{
  gchar *ret = NULL;
  GError *error = NULL;

  g_return_val_if_fail (entry, NULL);

  ret = g_key_file_get_locale_string (entry,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_ICON,
                                      NULL,
                                      &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  return ret;
}

static gchar *
desktop_file_get_comment (GKeyFile *entry)
{
  gchar *ret = NULL;
  GError *error = NULL;

  g_return_val_if_fail (entry, NULL);

  ret = g_key_file_get_locale_string (entry,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_COMMENT,
                                      NULL,
                                      &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  return ret;
}

MnbLauncherEntry *
mnb_launcher_entry_create (const gchar *desktop_file_path)
{
  MnbLauncherEntry  *self;
  GKeyFile          *desktop_file;
  GError            *error;

  g_return_val_if_fail (desktop_file_path, NULL);
  g_return_val_if_fail (*desktop_file_path, NULL);

  desktop_file = g_key_file_new ();
  error = NULL;
  g_key_file_load_from_file (desktop_file,
                             desktop_file_path,
                             G_KEY_FILE_NONE,
                             &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      self = NULL;
    }
  else
    {
      self = g_slice_alloc (sizeof (MnbLauncherEntry));
      self->desktop_file_path = g_strdup (desktop_file_path);
      self->name = desktop_file_get_name (desktop_file);
      self->exec = desktop_file_get_exec (desktop_file);
      self->icon = desktop_file_get_icon (desktop_file);
      self->comment = desktop_file_get_comment (desktop_file);
    }

  g_key_file_free (desktop_file);

  return self;
}

static MnbLauncherEntry *
mnb_launcher_entry_copy (MnbLauncherEntry *self)
{
  MnbLauncherEntry *copy;

  copy = g_slice_dup (MnbLauncherEntry, self);
  copy->desktop_file_path = g_strdup (self->desktop_file_path);
  copy->name = g_strdup (self->name);
  copy->exec = g_strdup (self->exec);
  copy->icon = g_strdup (self->icon);
  copy->comment = g_strdup (self->comment);

  return copy;
}

void
mnb_launcher_entry_free (MnbLauncherEntry *self)
{
  g_return_if_fail (self);

  g_free (self->desktop_file_path);
  g_free (self->name);
  g_free (self->exec);
  g_free (self->icon);
  g_free (self->comment);
  g_slice_free (MnbLauncherEntry, self);
}

const gchar *
mnb_launcher_entry_get_desktop_file_path (MnbLauncherEntry *self)
{
  g_return_val_if_fail (self, NULL);

  return self->desktop_file_path;
}

const gchar *
mnb_launcher_entry_get_name (MnbLauncherEntry *self)
{
  g_return_val_if_fail (self, NULL);

  return self->name;
}

const gchar *
mnb_launcher_entry_get_exec (MnbLauncherEntry *self)
{
  g_return_val_if_fail (self, NULL);

  return self->exec;
}
const gchar *
mnb_launcher_entry_get_icon (MnbLauncherEntry *self)
{
  g_return_val_if_fail (self, NULL);

  return self->icon;
}

const gchar *
mnb_launcher_entry_get_comment (MnbLauncherEntry *self)
{
  g_return_val_if_fail (self, NULL);

  return self->comment;
}

GType
mnb_launcher_entry_get_type (void)
{
  static GType _type = 0;

  if (_type == 0)
    _type = g_boxed_type_register_static (N_("MnbLauncherEntry"),
                                          (GBoxedCopyFunc) mnb_launcher_entry_copy,
                                          (GBoxedFreeFunc) mnb_launcher_entry_free);

  return _type;
}


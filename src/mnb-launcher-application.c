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
#include "mnb-launcher-application.h"

G_DEFINE_TYPE (MnbLauncherApplication, mnb_launcher_application, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_LAUNCHER_APPLICATION, MnbLauncherApplicationPrivate))

typedef struct {
  gchar     *desktop_file;
  gchar     *name;
  gchar     *executable;
  gchar     *icon;
  gchar     *description;
  gboolean   bookmark;
} MnbLauncherApplicationPrivate;

enum
{
  PROP_0,

  PROP_NAME,
  PROP_ICON,
  PROP_DESCRIPTION,
  PROP_EXECUTABLE,
  PROP_DESKTOP_FILE,

  PROP_BOOKMARK
};

/*
 * Desktop file utils.
 */

static gchar *
desktop_file_get_name (GKeyFile *application)
{
  gchar *ret = NULL;
  GError *error = NULL;

  g_return_val_if_fail (application, NULL);

  ret = g_key_file_get_locale_string (application,
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
  ret = g_key_file_get_locale_string (application,
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
 * Get executable from menu application and check it's available in the path.
 * Returns: absolute path if found, otherwise NULL.
 */
static gchar *
desktop_file_get_exec (GKeyFile *application)
{
  gchar   *exec = NULL;
  gchar   *binary = NULL;
  gint     argc;
  gchar  **argv;
  GError  *error = NULL;

  g_return_val_if_fail (application, NULL);

  error = NULL;
  exec = g_key_file_get_value (application,
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
desktop_file_get_icon (GKeyFile *application)
{
  gchar *ret = NULL;
  GError *error = NULL;

  g_return_val_if_fail (application, NULL);

  ret = g_key_file_get_locale_string (application,
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
desktop_file_get_comment (GKeyFile *application)
{
  gchar *ret = NULL;
  GError *error = NULL;

  g_return_val_if_fail (application, NULL);

  ret = g_key_file_get_locale_string (application,
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

static void
_set_property (GObject      *gobject,
               guint         prop_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (gobject);

  switch (prop_id)
    {
      case PROP_NAME:
        g_free (priv->name);
        priv->name = g_value_dup_string (value);
        g_object_notify (gobject, "name");
        break;
      case PROP_ICON:
        g_free (priv->icon);
        priv->icon = g_value_dup_string (value);
        g_object_notify (gobject, "icon");
        break;
      case PROP_DESCRIPTION:
        g_free (priv->description);
        priv->description = g_value_dup_string (value);
        g_object_notify (gobject, "description");
        break;
      case PROP_EXECUTABLE:
        g_free (priv->executable);
        priv->executable = g_value_dup_string (value);
        g_object_notify (gobject, "executable");
        break;
      case PROP_DESKTOP_FILE:
        g_free (priv->desktop_file);
        priv->desktop_file = g_value_dup_string (value);
        g_object_notify (gobject, "desktop-file");
        break;
      case PROP_BOOKMARK:
        priv->bookmark = g_value_get_boolean (value);
        g_object_notify (gobject, "bookmark");
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
        break;
    }
}

static void
_get_property (GObject    *gobject,
               guint       prop_id,
               GValue     *value,
               GParamSpec *pspec)
{
  MnbLauncherApplication *self = MNB_LAUNCHER_APPLICATION (gobject);

  switch (prop_id)
    {
      case PROP_NAME:
        g_value_set_string (value,
                            mnb_launcher_application_get_name (self));
        break;
      case PROP_ICON:
        g_value_set_string (value,
                            mnb_launcher_application_get_icon (self));
        break;
      case PROP_DESCRIPTION:
        g_value_set_string (value,
                            mnb_launcher_application_get_description (self));
        break;
      case PROP_EXECUTABLE:
        g_value_set_string (value,
                            mnb_launcher_application_get_executable (self));
        break;
      case PROP_DESKTOP_FILE:
        g_value_set_string (value,
                            mnb_launcher_application_get_desktop_file (self));
        break;
      case PROP_BOOKMARK:
        g_value_set_boolean (value,
                             mnb_launcher_application_get_bookmark (self));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
        break;
    }
}

static void
_dispose (GObject *gobject)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (gobject);

  if (priv->desktop_file)
    {
      g_free (priv->desktop_file);
      priv->desktop_file = NULL;
    }

  if (priv->name)
    {
      g_free (priv->name);
      priv->name = NULL;
    }

  if (priv->executable)
    {
      g_free (priv->executable);
      priv->executable = NULL;
    }

  if (priv->icon)
    {
      g_free (priv->icon);
      priv->icon = NULL;
    }

  if (priv->description)
    {
      g_free (priv->description);
      priv->description = NULL;
    }

  G_OBJECT_CLASS (mnb_launcher_application_parent_class)->dispose (gobject);
}

static void
mnb_launcher_application_class_init (MnbLauncherApplicationClass *klass)
{
  GObjectClass  *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec    *pspec;

  g_type_class_add_private (klass, sizeof (MnbLauncherApplicationPrivate));

  gobject_class->set_property = _set_property;
  gobject_class->get_property = _get_property;
  gobject_class->dispose      = _dispose;

  pspec = g_param_spec_string ("name",
                               "Name",
                               "Application name",
                               "Unnamed",
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_NAME, pspec);

  pspec = g_param_spec_string ("icon",
                               "Icon",
                               "Application icon",
                               "applications-other",
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_ICON, pspec);

  pspec = g_param_spec_string ("description",
                               "Description",
                               "Application description",
                               "<Unknown>",
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_DESCRIPTION, pspec);

  pspec = g_param_spec_string ("executable",
                               "Executable",
                               "Application executable",
                               NULL,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_EXECUTABLE, pspec);

  pspec = g_param_spec_string ("desktop-file",
                               "Desktop file",
                               "Path to desktop file",
                               NULL,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_DESKTOP_FILE, pspec);

  pspec = g_param_spec_boolean ("bookmark",
                                "Bookmark",
                                "Whether the application bookmarked",
                                FALSE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_BOOKMARK, pspec);
}

static void
mnb_launcher_application_init (MnbLauncherApplication *self)
{}

MnbLauncherApplication *
mnb_launcher_application_new (const gchar *name,
                        const gchar *icon,
                        const gchar *description,
                        const gchar *executable,
                        const gchar *desktop_file)
{
  MnbLauncherApplication *self;

  self = g_object_new (MNB_TYPE_LAUNCHER_APPLICATION,
                       "name", name,
                       "icon", icon,
                       "description", description,
                       "executable", executable,
                       "desktop-file", desktop_file,
                       NULL);

  return self;
}

MnbLauncherApplication *
mnb_launcher_application_new_from_desktop_file (const gchar *desktop_file)
{
  MnbLauncherApplication  *self = NULL;
  GKeyFile          *key_file;
  GError            *error = NULL;

  g_return_val_if_fail (desktop_file, NULL);
  g_return_val_if_fail (*desktop_file, NULL);

  key_file = g_key_file_new ();
  error = NULL;
  g_key_file_load_from_file (key_file,
                             desktop_file,
                             G_KEY_FILE_NONE,
                             &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }
  else
    {
      gchar *name = desktop_file_get_name (key_file);
      gchar *icon = desktop_file_get_icon (key_file);
      gchar *description = desktop_file_get_comment (key_file);
      gchar *executable = desktop_file_get_exec (key_file);

      self = mnb_launcher_application_new (name, icon, description, executable, desktop_file);

      g_free (name);
      g_free (icon);
      g_free (description);
      g_free (executable);
    }

  g_key_file_free (key_file);

  return self;
}

const gchar *
mnb_launcher_application_get_desktop_file (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, NULL);

  return priv->desktop_file;
}

const gchar *
mnb_launcher_application_get_name (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, NULL);

  return priv->name;
}

const gchar *
mnb_launcher_application_get_executable (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, NULL);

  return priv->executable;
}
const gchar *
mnb_launcher_application_get_icon (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, NULL);

  return priv->icon;
}

const gchar *
mnb_launcher_application_get_description (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, NULL);

  return priv->description;
}

gboolean
mnb_launcher_application_get_bookmark (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, FALSE);

  return priv->bookmark;
}

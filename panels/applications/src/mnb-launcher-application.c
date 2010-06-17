/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
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
  gboolean   bookmarked;
} MnbLauncherApplicationPrivate;

enum
{
  PROP_0,

  PROP_NAME,
  PROP_ICON,
  PROP_DESCRIPTION,
  PROP_EXECUTABLE,
  PROP_DESKTOP_FILE,

  PROP_BOOKMARKED
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
  switch (prop_id)
    {
      case PROP_NAME:
        mnb_launcher_application_set_name (MNB_LAUNCHER_APPLICATION (gobject),
                                           g_value_get_string (value));
        break;
      case PROP_ICON:
        mnb_launcher_application_set_icon (MNB_LAUNCHER_APPLICATION (gobject),
                                           g_value_get_string (value));
        break;
      case PROP_DESCRIPTION:
        mnb_launcher_application_set_description (MNB_LAUNCHER_APPLICATION (gobject),
                                                  g_value_get_string (value));
        break;
      case PROP_EXECUTABLE:
        mnb_launcher_application_set_executable (MNB_LAUNCHER_APPLICATION (gobject),
                                                 g_value_get_string (value));
        break;
      case PROP_DESKTOP_FILE:
        mnb_launcher_application_set_desktop_file (MNB_LAUNCHER_APPLICATION (gobject),
                                                   g_value_get_string (value));
        break;
      case PROP_BOOKMARKED:
        mnb_launcher_application_set_bookmarked (MNB_LAUNCHER_APPLICATION (gobject),
                                                 g_value_get_boolean (value));
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
      case PROP_BOOKMARKED:
        g_value_set_boolean (value,
                             mnb_launcher_application_get_bookmarked (self));
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

  pspec = g_param_spec_boolean ("bookmarked",
                                "Bookmark",
                                "Whether the application bookmarked",
                                FALSE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, PROP_BOOKMARKED, pspec);
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

MnbLauncherApplication *
mnb_launcher_application_new_from_cache (const gchar **attribute_names,
                                         const gchar **attribute_values)
{
  const gchar *name = NULL;
  const gchar *icon = NULL;
  const gchar *description = NULL;
  const gchar *executable = NULL;
  const gchar *desktop_file = NULL;
  gboolean     bookmarked = FALSE;
  guint        i = 0;

  while (attribute_names[i])
    {
      if (0 == g_strcmp0 ("name", attribute_names[i]))
        name = attribute_values[i];
      if (0 == g_strcmp0 ("icon", attribute_names[i]))
        icon = attribute_values[i];
      if (0 == g_strcmp0 ("description", attribute_names[i]))
        description = attribute_values[i];
      if (0 == g_strcmp0 ("executable", attribute_names[i]))
        executable = attribute_values[i];
      if (0 == g_strcmp0 ("desktop-file", attribute_names[i]))
        desktop_file = attribute_values[i];
      if (0 == g_strcmp0 ("bookmarked", attribute_names[i]))
        bookmarked = (0 == g_strcmp0 ("true", attribute_values[i])) ?
                        TRUE :
                        FALSE;
      i++;
    }

  g_return_val_if_fail (desktop_file, NULL);

  return g_object_new (MNB_TYPE_LAUNCHER_APPLICATION,
                       "name", name,
                       "icon", icon,
                       "description", description,
                       "executable", executable,
                       "desktop-file", desktop_file,
                       "bookmarked", bookmarked,
                        NULL);
}

const gchar *
mnb_launcher_application_get_desktop_file (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, NULL);

  return priv->desktop_file;
}

void
mnb_launcher_application_set_desktop_file (MnbLauncherApplication *self,
                                           const gchar            *desktop_file)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (priv);

  g_free (priv->desktop_file);
  priv->desktop_file = g_strdup (desktop_file);
  g_object_notify (G_OBJECT (self), "desktop-file");
}

const gchar *
mnb_launcher_application_get_name (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, NULL);

  return priv->name;
}

void
mnb_launcher_application_set_name (MnbLauncherApplication *self,
                                   const gchar            *name)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (priv);

  g_free (priv->name);
  priv->name = g_strdup (name);
  g_object_notify (G_OBJECT (self), "name");
}

const gchar *
mnb_launcher_application_get_executable (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, NULL);

  return priv->executable;
}

void
mnb_launcher_application_set_executable (MnbLauncherApplication *self,
                                         const gchar            *executable)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (priv);

  g_free (priv->executable);
  priv->executable = g_strdup (executable);
  g_object_notify (G_OBJECT (self), "executable");
}

const gchar *
mnb_launcher_application_get_icon (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, NULL);

  return priv->icon;
}

void
mnb_launcher_application_set_icon (MnbLauncherApplication *self,
                                   const gchar            *icon)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (priv);

  g_free (priv->icon);
  priv->icon = g_strdup (icon);

  if (priv->icon && !g_path_is_absolute (priv->icon))
    {
      /* Cut off suffix. GDesktopAppInfo does this and we want to be
       * consistent with the myzone. */
      gchar *suffix = strrchr (priv->icon, '.');
      if (suffix &&
            (0 == g_ascii_strcasecmp (suffix, ".png") ||
             0 == g_ascii_strcasecmp (suffix, ".svg") ||
             0 == g_ascii_strcasecmp (suffix, ".xpm")))
        {
          *suffix = '\0';
        }
    }

  g_object_notify (G_OBJECT (self), "icon");
}

const gchar *
mnb_launcher_application_get_description (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, NULL);

  return priv->description;
}

void
mnb_launcher_application_set_description (MnbLauncherApplication *self,
                                          const gchar            *description)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (priv);

  g_free (priv->description);
  priv->description = g_strdup (description);
  g_object_notify (G_OBJECT (self), "description");
}

gboolean
mnb_launcher_application_get_bookmarked (MnbLauncherApplication *self)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (priv, FALSE);

  return priv->bookmarked;
}

void
mnb_launcher_application_set_bookmarked (MnbLauncherApplication *self,
                                         gboolean                bookmarked)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (priv);

  if (priv->bookmarked != bookmarked)
    {
      priv->bookmarked = bookmarked;
      g_object_notify (G_OBJECT (self), "bookmarked");
    }
}

void
mnb_launcher_application_write_xml (MnbLauncherApplication const *self,
                                    FILE                         *fp)
{
  MnbLauncherApplicationPrivate *priv = GET_PRIVATE (self);
  gchar *text;

  text = g_markup_printf_escaped ("    <application desktop-file=\"%s\" bookmarked=\"%s\">\n",
                                  priv->desktop_file,
                                  priv->bookmarked ? "true" : "false");
  fputs (text, fp);
  g_free (text);
  if (priv->name)
    {
      text = g_markup_printf_escaped ("      <name>%s</name>\n",
                                      priv->name);
      fputs (text, fp);
      g_free (text);
    }
  if (priv->executable)
    {
      text = g_markup_printf_escaped ("      <executable>%s</executable>\n",
                                      priv->executable);
      fputs (text, fp);
      g_free (text);
    }
  if (priv->icon)
    {
      text = g_markup_printf_escaped ("      <icon>%s</icon>\n",
                                      priv->icon);
      fputs (text, fp);
      g_free (text);
    }
  if (priv->description)
    {
      text = g_markup_printf_escaped ("      <description>%s</description>\n",
                                      priv->description);
      fputs (text, fp);
      g_free (text);
    }
  fputs   (    "    </application>\n", fp);
}


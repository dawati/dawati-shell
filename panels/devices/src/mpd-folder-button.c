
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
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

#include "mpd-folder-button.h"
#include "config.h"

G_DEFINE_TYPE (MpdFolderButton, mpd_folder_button, MX_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FOLDER_BUTTON, MpdFolderButtonPrivate))

enum
{
  PROP_0,

  PROP_URI
};

typedef struct
{
  gchar *uri;
} MpdFolderButtonPrivate;

static void
_get_property (GObject    *object,
               guint       property_id,
               GValue     *value,
               GParamSpec *pspec)
{
  switch (property_id)
  {
  case PROP_URI:
    g_value_set_string (value, 
                        mpd_folder_button_get_uri (MPD_FOLDER_BUTTON (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               guint         property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_URI:
    mpd_folder_button_set_uri (MPD_FOLDER_BUTTON (object),
                               g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdFolderButtonPrivate *priv = GET_PRIVATE (object);

  if (priv->uri)
  {
    g_free (priv->uri);
    priv->uri = NULL;
  }

  G_OBJECT_CLASS (mpd_folder_button_parent_class)->dispose (object);
}

static void
mpd_folder_button_class_init (MpdFolderButtonClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GParamFlags    param_flags;

  g_type_class_add_private (klass, sizeof (MpdFolderButtonPrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_URI,
                                   g_param_spec_string ("uri",
                                                        "URI",
                                                        "Folder URI",
                                                        NULL,
                                                        param_flags));
}

static void
mpd_folder_button_init (MpdFolderButton *self)
{
}

ClutterActor *
mpd_folder_button_new (void)
{
  return g_object_new (MPD_TYPE_FOLDER_BUTTON, NULL);
}

gchar const *
mpd_folder_button_get_uri (MpdFolderButton const *self)
{
  MpdFolderButtonPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_FOLDER_BUTTON (self), NULL);

  return priv->uri;
}

void
mpd_folder_button_set_uri (MpdFolderButton  *self,
                           gchar const      *uri)
{
  MpdFolderButtonPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_FOLDER_BUTTON (self));

  if (0 != g_strcmp0 (priv->uri, uri))
  {
    if (priv->uri)
    {
      g_free (priv->uri);
      priv->uri = NULL;
    }

    if (uri)
    {
      priv->uri = g_strdup (uri);
    }

    g_object_notify (G_OBJECT (self), "uri");
  }
}


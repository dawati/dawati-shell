/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <gtk/gtk.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "penge-app-tile.h"
#include "penge-utils.h"

G_DEFINE_TYPE (PengeAppTile, penge_app_tile, NBTK_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_APP_TILE, PengeAppTilePrivate))

typedef struct _PengeAppTilePrivate PengeAppTilePrivate;

struct _PengeAppTilePrivate {
  ClutterActor *tex;
  GtkIconTheme *icon_theme;
  GAppInfo *app_info;
  gchar *bookmark;
};

enum
{
  PROP_0,
  PROP_BOOKMARK
};

#define ICON_SIZE 48

void _icon_theme_changed_cb (GtkIconTheme *icon_theme,
                             gpointer      userdata);

static void
penge_app_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (object);


  switch (property_id) {
    case PROP_BOOKMARK:
      g_value_set_string (value, priv->bookmark);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_app_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_BOOKMARK:
      priv->bookmark = g_value_dup_string (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_app_tile_dispose (GObject *object)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (object);

  if (priv->app_info)
  {
    g_signal_handlers_disconnect_by_func (priv->icon_theme,
                                          _icon_theme_changed_cb,
                                          object);
    g_object_unref (priv->app_info);
    priv->app_info = NULL;
  }

  G_OBJECT_CLASS (penge_app_tile_parent_class)->dispose (object);
}

static void
penge_app_tile_finalize (GObject *object)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (object);

  g_free (priv->bookmark);

  G_OBJECT_CLASS (penge_app_tile_parent_class)->finalize (object);
}

static void
_update_icon_from_icon_theme (PengeAppTile *tile)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (tile);
  const gchar *path;
  gchar *icon_path;
  GError *error = NULL;
  GtkIconInfo *info;
  GIcon *icon;

  icon = g_app_info_get_icon (priv->app_info);

  if (G_IS_FILE_ICON (icon))
  {
    icon_path = g_icon_to_string (icon);

    if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->tex),
                                        icon_path,
                                        &error))
    {
      g_warning (G_STRLOC ": Error loading texture from file: %s",
                 error->message);
      g_clear_error (&error);
    }

    g_free (icon_path);
  } else {
    info = gtk_icon_theme_lookup_by_gicon (priv->icon_theme,
                                           icon,
                                           ICON_SIZE,
                                           GTK_ICON_LOOKUP_GENERIC_FALLBACK);

    if (!info)
    {
      /* try falling back */
      info = gtk_icon_theme_lookup_icon (priv->icon_theme,
                                         "applications-other",
                                         ICON_SIZE,
                                         GTK_ICON_LOOKUP_GENERIC_FALLBACK);
    }

    if (info)
    {
      path = gtk_icon_info_get_filename (info);

      if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->tex),
                                          path,
                                          &error))
      {
        g_warning (G_STRLOC ": Error loading texture from file: %s",
                   error->message);
        g_clear_error (&error);
      }
    }
  }
}

void
_icon_theme_changed_cb (GtkIconTheme *icon_theme,
                        gpointer      userdata)
{
  PengeAppTile *tile = (PengeAppTile *)userdata;

  _update_icon_from_icon_theme (tile);
}

static void
penge_app_tile_constructed (GObject *object)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (object);
  GKeyFile *kf;
  gchar *path;
  GError *error = NULL;
  gchar *name = NULL;

  g_return_if_fail (priv->bookmark);

  if (!priv->bookmark)
    return;

  priv->icon_theme = gtk_icon_theme_get_default ();
  g_signal_connect (priv->icon_theme,
                    "changed",
                    (GCallback)_icon_theme_changed_cb,
                    object);

  path = g_filename_from_uri (priv->bookmark, NULL, &error);

  if (path)
  {
    priv->app_info = G_APP_INFO (g_desktop_app_info_new_from_filename (path));
    kf = g_key_file_new ();
    if (!g_key_file_load_from_file (kf,
                                    path,
                                    G_KEY_FILE_NONE,
                                    &error))
    {
      g_warning (G_STRLOC ": Error getting a key file for path: %s",
                 error->message);
      g_clear_error (&error);
    } else {
      name = g_key_file_get_locale_string (kf,
                                           G_KEY_FILE_DESKTOP_GROUP,
                                           G_KEY_FILE_DESKTOP_KEY_GENERIC_NAME,
                                           NULL,
                                           NULL);

      if (!name)
      {
        name = g_key_file_get_locale_string (kf,
                                             G_KEY_FILE_DESKTOP_GROUP,
                                             G_KEY_FILE_DESKTOP_KEY_NAME,
                                             NULL,
                                             NULL);
      }

      nbtk_widget_set_tooltip_text (NBTK_WIDGET (object),
                                    name);
      g_free (name);
    }
    g_key_file_free (kf);

    g_free (path);
  }

  if (error)
  {
    g_warning (G_STRLOC ": Error getting info from bookmark: %s",
               error->message);
    g_clear_error (&error);
  }

  _update_icon_from_icon_theme ((PengeAppTile *)object);
}

static void
penge_app_tile_class_init (PengeAppTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeAppTilePrivate));

  object_class->get_property = penge_app_tile_get_property;
  object_class->set_property = penge_app_tile_set_property;
  object_class->dispose = penge_app_tile_dispose;
  object_class->finalize = penge_app_tile_finalize;
  object_class->constructed = penge_app_tile_constructed;

  pspec = g_param_spec_string ("bookmark",
                               "bookmark",
                               "bookmark",
                               NULL,
                               G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_BOOKMARK, pspec);
}

static void
_button_clicked_cb (NbtkButton *button,
                    gpointer    userdata)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (userdata);
  GError *error = NULL;
  gchar *path = NULL;

  path = g_filename_from_uri (priv->bookmark, NULL, &error);

  if (!path)
  {
    if (error)
    {
      g_warning (G_STRLOC ": Error getting path from uri: %s",
                 error->message);
      g_clear_error (&error);
    }

    return;
  }

  if (penge_utils_launch_for_desktop_file ((ClutterActor *)button, path))
    penge_utils_signal_activated ((ClutterActor *)button);
  else
    g_warning (G_STRLOC ": Unable to launch for desktop file: %s", path);

  g_free (path);
}

static void
penge_app_tile_init (PengeAppTile *self)
{
  PengeAppTilePrivate *priv = GET_PRIVATE (self);

  priv->tex = clutter_texture_new ();
  clutter_actor_set_size (priv->tex, ICON_SIZE, ICON_SIZE);

  nbtk_bin_set_child (NBTK_BIN (self),
                      priv->tex);
  g_signal_connect (self,
                    "clicked",
                    (GCallback)_button_clicked_cb,
                    self);
}



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

#include <gtk/gtk.h>
#include "mpd-folder-button.h"
#include "config.h"

#define ICON_SIZE 48

/* Overriding the button's "label" property. */

static gchar const *
mpd_folder_button_get_label (MpdFolderButton *self);

static void
mpd_folder_button_set_label (MpdFolderButton  *self,
                             gchar const      *label);

G_DEFINE_TYPE (MpdFolderButton, mpd_folder_button, MX_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FOLDER_BUTTON, MpdFolderButtonPrivate))

enum
{
  PROP_0,

  PROP_LABEL,
  PROP_URI
};

typedef struct
{
  /* Managed by clutter */
  ClutterActor  *icon;
  ClutterActor  *label;

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
  case PROP_LABEL:
    g_value_set_string (value,
                        mpd_folder_button_get_label (MPD_FOLDER_BUTTON (object)));
    break;
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
  case PROP_LABEL:
    mpd_folder_button_set_label (MPD_FOLDER_BUTTON (object),
                                 g_value_get_string (value));
    break;
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

  /* Properties */

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  g_object_class_override_property (object_class, PROP_LABEL, "label");

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
  MpdFolderButtonPrivate *priv = GET_PRIVATE (self);
  ClutterActor  *vbox;
  ClutterText   *text;
  GtkIconInfo   *icon_info;
  gchar const   *filename;
  GError        *error = NULL;

  /* Box */
  vbox = mx_box_layout_new ();
  mx_box_layout_set_vertical (MX_BOX_LAYOUT (vbox), TRUE);
  mx_bin_set_child (MX_BIN (self), vbox);

  /* Icon */
  icon_info = gtk_icon_theme_lookup_icon (gtk_icon_theme_get_default (),
                                          "folder", ICON_SIZE,
                                          GTK_ICON_LOOKUP_NO_SVG);

  filename = gtk_icon_info_get_filename (icon_info);
  priv->icon = clutter_texture_new_from_file (filename, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
  clutter_actor_set_width (priv->icon, ICON_SIZE);
  clutter_actor_set_height (priv->icon, ICON_SIZE);
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), priv->icon);
  clutter_container_child_set (CLUTTER_CONTAINER (vbox), priv->icon,
                               "expand", FALSE,
                               "x-align", MX_ALIGN_MIDDLE,
                               "x-fill", FALSE,
                               "y-align", MX_ALIGN_START,
                               "y-fill", FALSE,
                               NULL);
  gtk_icon_info_free (icon_info);

  /* Label */
  priv->label = mx_label_new ("");
  text = (ClutterText *) mx_label_get_clutter_text (MX_LABEL (priv->label));
  clutter_text_set_line_wrap_mode (text, PANGO_WRAP_WORD_CHAR);
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), priv->label);
  clutter_container_child_set (CLUTTER_CONTAINER (vbox), priv->label,
                               "expand", FALSE,
                               "x-align", MX_ALIGN_MIDDLE,
                               "x-fill", FALSE,
                               "y-align", MX_ALIGN_START,
                               "y-fill", FALSE,
                               NULL);
}

ClutterActor *
mpd_folder_button_new (void)
{
  return g_object_new (MPD_TYPE_FOLDER_BUTTON, NULL);
}

static gchar const *
mpd_folder_button_get_label (MpdFolderButton *self)
{
  MpdFolderButtonPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_FOLDER_BUTTON (self), NULL);

  return mx_label_get_text (MX_LABEL (priv->label));
}

static void
mpd_folder_button_set_label (MpdFolderButton  *self,
                             gchar const      *text)
{
  MpdFolderButtonPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_FOLDER_BUTTON (self));

  if (0 != g_strcmp0 (text,
                      mx_label_get_text (MX_LABEL (priv->label))))
  {
    mx_label_set_text (MX_LABEL (priv->label), text);
    g_object_notify (G_OBJECT (self), "label");
  }
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


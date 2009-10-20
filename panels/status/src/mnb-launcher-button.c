/*
 * Copyright (C) 2009 Intel Corporation.
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
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

#include "mnb-launcher-button.h"

G_DEFINE_TYPE (MnbLauncherButton, mnb_launcher_button, NBTK_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButtonPrivate))

typedef struct _MnbLauncherButtonPrivate MnbLauncherButtonPrivate;

struct _MnbLauncherButtonPrivate {
  gchar *app_name;
  GAppInfo *app_info;
  ClutterActor *tex;
  NbtkWidget *label;
  GtkIconTheme *icon_theme;
  NbtkWidget *inner_table;
};

enum
{
  PROP_0,
  PROP_APP_NAME
};

#define ICON_SIZE 48

static void mnb_launcher_button_update_icon (MnbLauncherButton *button);

static void
mnb_launcher_button_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  MnbLauncherButtonPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_APP_NAME:
      g_value_set_string (value, priv->app_name);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_launcher_button_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  MnbLauncherButtonPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_APP_NAME:
      priv->app_name = g_value_dup_string (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_launcher_button_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_launcher_button_parent_class)->dispose (object);
}

static void
mnb_launcher_button_finalize (GObject *object)
{
  MnbLauncherButtonPrivate *priv = GET_PRIVATE (object);

  g_free (priv->app_name);

  G_OBJECT_CLASS (mnb_launcher_button_parent_class)->finalize (object);
}

static void
mnb_launcher_button_constructed (GObject *object)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (object);
  MnbLauncherButtonPrivate *priv = GET_PRIVATE (self);

  priv->app_info = (GAppInfo *)g_desktop_app_info_new (priv->app_name);

  nbtk_label_set_text (NBTK_LABEL (priv->label),
                       g_app_info_get_description (priv->app_info));

  mnb_launcher_button_update_icon (self);

  if (G_OBJECT_CLASS (mnb_launcher_button_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_launcher_button_parent_class)->constructed (object);
}

static void
mnb_launcher_button_class_init (MnbLauncherButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MnbLauncherButtonPrivate));

  object_class->get_property = mnb_launcher_button_get_property;
  object_class->set_property = mnb_launcher_button_set_property;
  object_class->dispose = mnb_launcher_button_dispose;
  object_class->finalize = mnb_launcher_button_finalize;
  object_class->constructed = mnb_launcher_button_constructed;

  pspec = g_param_spec_string ("app-name",
                               "Application name",
                               "Application to build launcher for",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_APP_NAME, pspec);
}

static void
mnb_launcher_button_update_icon (MnbLauncherButton *button)
{
  MnbLauncherButtonPrivate *priv = GET_PRIVATE (button);

  GError *error = NULL;
  GtkIconInfo *icon_info;
  GIcon *icon;

  if (!priv->app_info)
    return;

  icon = g_app_info_get_icon (priv->app_info);
  icon_info = gtk_icon_theme_lookup_by_gicon (priv->icon_theme,
                                              icon,
                                              ICON_SIZE,
                                              GTK_ICON_LOOKUP_GENERIC_FALLBACK);

  clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->tex),
                                 gtk_icon_info_get_filename (icon_info),
                                 &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error opening icon: %s",
               error->message);
    g_clear_error (&error);
  }
}

static void
_icon_theme_changed_cb (GtkIconTheme *icon_theme,
                        gpointer      userdata)
{
  mnb_launcher_button_update_icon (MNB_LAUNCHER_BUTTON (userdata));
}

static void
mnb_launcher_button_init (MnbLauncherButton *self)
{
  MnbLauncherButtonPrivate *priv = GET_PRIVATE (self);

  priv->inner_table = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (priv->inner_table), 8);
  nbtk_bin_set_child (NBTK_BIN (self), CLUTTER_ACTOR (priv->inner_table));

  priv->tex = clutter_texture_new ();
  clutter_actor_set_size (priv->tex, ICON_SIZE, ICON_SIZE);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->inner_table),
                                        priv->tex,
                                        0, 0,
                                        "x-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "y-expand", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);
  priv->label = nbtk_label_new (NULL);
  clutter_actor_set_name (CLUTTER_ACTOR (priv->label),
                          "launcher-app-description");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->inner_table),
                                        CLUTTER_ACTOR (priv->label),
                                        0, 1,
                                        "x-expand", TRUE,
                                        "x-fill", FALSE,
                                        "y-expand", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

  priv->icon_theme = gtk_icon_theme_get_default ();

  /* Listen for the theme change */
  g_signal_connect (priv->icon_theme,
                    "changed",
                    (GCallback)_icon_theme_changed_cb,
                    self);
}


NbtkWidget *
mnb_launcher_button_new (const gchar *desktop_file_name)
{
  return g_object_new (MNB_TYPE_LAUNCHER_BUTTON,
                       "app-name", desktop_file_name,
                       NULL);
}

gboolean
mnb_launcher_button_launch (MnbLauncherButton *button)
{
  MnbLauncherButtonPrivate *priv = GET_PRIVATE (button);
  GError *error = NULL;
  GAppLaunchContext *context;

  context = G_APP_LAUNCH_CONTEXT (gdk_app_launch_context_new ());

  if (g_app_info_launch (priv->app_info, NULL, context, &error))
  {
    g_object_unref (context);
    return TRUE;
  }

  if (error)
  {
    g_warning (G_STRLOC ": Error launching application): %s",
                 error->message);
    g_clear_error (&error);
  }

  g_object_unref (context);
  return FALSE;
}



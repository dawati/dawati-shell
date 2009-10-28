 /*
 * Copyright (C) 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>

#include <penge/penge-utils.h>

#include "penge-people-placeholder-tile.h"

G_DEFINE_TYPE (PengePeoplePlaceholderTile, penge_people_placeholder_tile, NBTK_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_PLACEHOLDER_TILE, PengePeoplePlaceholderTilePrivate))

#define ICON_SIZE 48
#define TILE_WIDTH 170.0f
#define TILE_HEIGHT 115.0f

typedef struct _PengePeoplePlaceholderTilePrivate PengePeoplePlaceholderTilePrivate;

struct _PengePeoplePlaceholderTilePrivate {
  NbtkWidget *inner_table;
};

static void
penge_people_placeholder_tile_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_people_placeholder_tile_parent_class)->dispose (object);
}

static void
penge_people_placeholder_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_placeholder_tile_parent_class)->finalize (object);
}

static void
penge_people_placeholder_tile_get_preferred_width (ClutterActor *self,
                                                   gfloat        for_height,
                                                   gfloat       *min_width_p,
                                                   gfloat       *natural_width_p)
{
  if (min_width_p)
    *min_width_p = TILE_WIDTH * 2;

  if (natural_width_p)
    *natural_width_p = TILE_WIDTH * 2;
}

static void
penge_people_placeholder_tile_class_init (PengePeoplePlaceholderTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengePeoplePlaceholderTilePrivate));

  object_class->dispose = penge_people_placeholder_tile_dispose;
  object_class->finalize = penge_people_placeholder_tile_finalize;

  actor_class->get_preferred_width = penge_people_placeholder_tile_get_preferred_width;
}

static void
_button_clicked_cb (NbtkButton *button,
                    gpointer    userdata)
{
  GAppInfo *app_info = (GAppInfo *)userdata;

  if (penge_utils_launch_by_command_line (CLUTTER_ACTOR (button),
                                          g_app_info_get_executable (app_info)))
    penge_utils_signal_activated (CLUTTER_ACTOR (button));
  else
    g_warning (G_STRLOC ": Unable to launch web services settings");
}

static void
penge_people_placeholder_tile_init (PengePeoplePlaceholderTile *self)
{
  PengePeoplePlaceholderTilePrivate *priv = GET_PRIVATE (self);
  NbtkWidget *label;
  ClutterActor *tex;
  GtkIconTheme *icon_theme;
  GtkIconInfo *icon_info;
  GAppInfo *app_info;
  GError *error = NULL;
  GIcon *icon;
  ClutterActor *tmp_text;

  priv->inner_table = nbtk_table_new ();
  nbtk_bin_set_child (NBTK_BIN (self), (ClutterActor *)priv->inner_table);
  nbtk_bin_set_fill (NBTK_BIN (self), TRUE, TRUE);

  label = nbtk_label_new (_("Things that your friends do online will appear here. " \
                            "Activate your accounts now!"));
  clutter_actor_set_name ((ClutterActor *)label, "penge-no-content-main-message");

  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->inner_table),
                                        (ClutterActor *)label,
                                        0, 0,
                                        "x-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-expand", TRUE,
                                        "y-fill", TRUE,
                                        "col-span", 2,
                                        NULL);

  app_info = (GAppInfo *)g_desktop_app_info_new ("bisho.desktop");

  if (app_info)
  {
    icon_theme = gtk_icon_theme_get_default ();

    icon = g_app_info_get_icon (app_info);
    icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme,
                                                icon,
                                                ICON_SIZE,
                                                GTK_ICON_LOOKUP_GENERIC_FALLBACK);

    tex = clutter_texture_new_from_file (gtk_icon_info_get_filename (icon_info),
                                         &error);

    if (!tex)
    {
      g_warning (G_STRLOC ": Error opening icon: %s",
                 error->message);
      g_clear_error (&error);
    } else {
      clutter_actor_set_size (tex, ICON_SIZE, ICON_SIZE);
      nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->inner_table),
                                            tex,
                                            1, 0,
                                            "x-expand", FALSE,
                                            "x-fill", FALSE,
                                            "y-fill", FALSE,
                                            "y-expand", TRUE,
                                            NULL);
    }

    label = nbtk_label_new (g_app_info_get_name (app_info));
    clutter_actor_set_name ((ClutterActor *)label, "penge-no-content-other-message");
    nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->inner_table),
                                          (ClutterActor *)label,
                                          1, 1,
                                          "x-expand", TRUE,
                                          "x-fill", TRUE,
                                          "y-expand", TRUE,
                                          "y-fill", FALSE,
                                          NULL);
  }

  g_signal_connect (self,
                    "clicked",
                    (GCallback)_button_clicked_cb,
                    app_info);
}

ClutterActor *
penge_people_placeholder_tile_new (void)
{
  return g_object_new (PENGE_TYPE_PEOPLE_PLACEHOLDER_TILE, NULL);
}



/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
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

#include <config.h>
#include <glib/gi18n-lib.h>
#include <penge/penge-utils.h>

#include "penge-welcome-tile.h"

G_DEFINE_TYPE (PengeWelcomeTile, penge_welcome_tile, MX_TYPE_TABLE)

#define PLACEHOLDER_TEXT _("When you’ve set up your web " \
"accounts and used some files, they " \
"will appear here on myzone " \
"for easy access whenever you " \
"want. ")

#define TMP_PLACEHOLDER _("Play intro video")

#define WELCOME_VIDEO_FILENAME "/usr/share/videos/meego/meego-intro.ogv"

static void
penge_welcome_tile_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_welcome_tile_parent_class)->dispose (object);
}

static void
penge_welcome_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_welcome_tile_parent_class)->finalize (object);
}

static void
penge_welcome_tile_class_init (PengeWelcomeTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = penge_welcome_tile_dispose;
  object_class->finalize = penge_welcome_tile_finalize;
}

static void
_welcome_launcher_clicked_cb (MxButton *button,
                              gpointer  userdata)
{
  gchar *uri;
  const gchar *video_path;

  video_path =  WELCOME_VIDEO_FILENAME;

  uri = g_filename_to_uri (video_path, NULL, NULL);

  if (penge_utils_launch_for_uri ((ClutterActor *)button, uri))
  {
    penge_utils_signal_activated ((ClutterActor *)button);
  }

  g_free (uri);
}

static void
penge_welcome_tile_init (PengeWelcomeTile *tile)
{
  ClutterActor *label;
  ClutterActor *tmp_text;

  mx_stylable_set_style_class (MX_STYLABLE (tile),
                               "PengeWelcomeTile");

  label = mx_label_new_with_text (_("Welcome to MeeGo"));
  clutter_actor_set_name ((ClutterActor *)label,
                          "penge-welcome-primary-text");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);
  mx_table_add_actor_with_properties (MX_TABLE (tile),
                                      (ClutterActor *)label,
                                      0, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", TRUE,
                                      NULL);
  mx_table_set_row_spacing (MX_TABLE (tile), 12);

  label = mx_label_new_with_text (PLACEHOLDER_TEXT);


  clutter_actor_set_name ((ClutterActor *)label,
                          "penge-welcome-secondary-text");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);
  mx_table_add_actor_with_properties (MX_TABLE (tile),
                                      (ClutterActor *)label,
                                      1, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", TRUE,
                                      "y-fill", TRUE,
                                      NULL);

  {
    gchar *video_path;
    ClutterActor *launcher;
    ClutterActor *inner_table;
    ClutterActor *icon;

    video_path = WELCOME_VIDEO_FILENAME;

    if (g_file_test (video_path, G_FILE_TEST_EXISTS))
    {
      launcher = mx_button_new ();
      clutter_actor_set_name (launcher, "penge-welcome-launcher");

      inner_table = mx_table_new ();
      mx_bin_set_child (MX_BIN (launcher), inner_table);

      icon = mx_icon_new ();
      clutter_actor_set_name (icon, "penge-welcome-launcher-thumbnail");
      mx_table_add_actor_with_properties (MX_TABLE (inner_table),
                                          icon,
                                          0, 0,
                                          "x-expand", TRUE,
                                          "x-fill", TRUE,
                                          "y-expand", TRUE,
                                          "y-fill", TRUE,
                                          NULL);

      icon = mx_icon_new ();
      clutter_actor_set_name (icon, "penge-welcome-launcher-play-button");
      mx_table_add_actor_with_properties (MX_TABLE (inner_table),
                                          icon,
                                          0, 0,
                                          "x-expand", TRUE,
                                          "x-fill", FALSE,
                                          "y-expand", TRUE,
                                          "y-fill", FALSE,
                                          NULL);

      mx_table_add_actor_with_properties (MX_TABLE (tile),
                                          launcher,
                                          2, 0,
                                          "x-expand", FALSE,
                                          "x-fill", FALSE,
                                          "y-expand", FALSE,
                                          "y-fill", FALSE,
                                          "x-align", MX_ALIGN_START,
                                          NULL);

      g_signal_connect (launcher,
                        "clicked",
                        (GCallback)_welcome_launcher_clicked_cb,
                        NULL);
    }
  }
}

ClutterActor *
penge_welcome_tile_new (void)
{
  return g_object_new (PENGE_TYPE_WELCOME_TILE, NULL);
}



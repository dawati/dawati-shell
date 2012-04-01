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

#define PLACEHOLDER_IMAGE THEMEDIR "/default-message.png"

/*
#define TMP_PLACEHOLDER _("Play intro video")

#define WELCOME_VIDEO_FILENAME "/usr/share/videos/dawati/dawati-intro.ogv"
*/

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

/*
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
*/

static void
penge_welcome_tile_init (PengeWelcomeTile *tile)
{
  ClutterActor *placeholder_image;
  GError *error = NULL;

  placeholder_image = clutter_texture_new_from_file (PLACEHOLDER_IMAGE, &error);
  if (error != NULL)
    {
      g_warning ("Couldn't open the placeholder image %s", PLACEHOLDER_IMAGE);
      g_clear_error (&error);
    }
  else
    {
      ClutterActor *bin;

      bin = mx_frame_new ();

      clutter_container_add_actor (CLUTTER_CONTAINER (bin),
          placeholder_image);

      clutter_actor_set_name (bin,
                              "penge-welcome-placeholder-margin");
      clutter_actor_set_name (placeholder_image,
                              "penge-welcome-placeholder");
      clutter_actor_set_size (placeholder_image, 548, 247);

      mx_table_insert_actor_with_properties (MX_TABLE (tile),
                                             bin,
                                             0, 0,
                                             "x-expand", TRUE,
                                             "y-expand", TRUE,
                                             "x-fill", TRUE,
                                             "y-fill", TRUE,
                                             "x-align", MX_ALIGN_START,
                                             NULL);
      mx_bin_set_fill (MX_BIN (bin), TRUE, TRUE);
    }


  /* It's not to be shown
  if (g_file_test (WELCOME_VIDEO_FILENAME, G_FILE_TEST_EXISTS))
    {
      ClutterActor *launcher;
      ClutterActor *inner_table;
      ClutterActor *icon;

      launcher = mx_button_new ();
      clutter_actor_set_name (launcher, "penge-welcome-launcher");

      inner_table = mx_table_new ();
      mx_bin_set_child (MX_BIN (launcher), inner_table);

      icon = mx_icon_new ();
      clutter_actor_set_name (icon, "penge-welcome-launcher-thumbnail");
      mx_table_insert_actor_with_properties (MX_TABLE (inner_table),
                                          icon,
                                          0, 0,
                                          "x-expand", TRUE,
                                          "x-fill", TRUE,
                                          "y-expand", TRUE,
                                          "y-fill", TRUE,
                                          NULL);

      icon = mx_icon_new ();
      clutter_actor_set_name (icon, "penge-welcome-launcher-play-button");
      mx_table_insert_actor_with_properties (MX_TABLE (inner_table),
                                          icon,
                                          0, 0,
                                          "x-expand", TRUE,
                                          "x-fill", FALSE,
                                          "y-expand", TRUE,
                                          "y-fill", FALSE,
                                          NULL);

      mx_table_insert_actor_with_properties (MX_TABLE (tile),
                                          launcher,
                                          1, 0,
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
    */
}

ClutterActor *
penge_welcome_tile_new (void)
{
  return g_object_new (PENGE_TYPE_WELCOME_TILE, NULL);
}



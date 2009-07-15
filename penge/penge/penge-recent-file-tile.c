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

#include "penge-recent-file-tile.h"
#include "penge-magic-texture.h"
#include "penge-utils.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (PengeRecentFileTile, penge_recent_file_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_RECENT_FILE_TILE, PengeRecentFileTilePrivate))

typedef struct _PengeRecentFileTilePrivate PengeRecentFileTilePrivate;

struct _PengeRecentFileTilePrivate {
    gchar *thumbnail_path;
    GtkRecentInfo *info;
    NbtkWidget *details_overlay;
    NbtkWidget *details_filename_label;
    NbtkWidget *details_type_label;

    /* For the details fade in */
    ClutterTimeline *timeline;
    ClutterBehaviour *behave;
};

enum
{
  PROP_0,
  PROP_THUMBNAIL_PATH,
  PROP_INFO
};

static void
penge_recent_file_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_THUMBNAIL_PATH:
      g_value_set_string (value, priv->thumbnail_path);
      break;
    case PROP_INFO:
      g_value_set_pointer (value, priv->info);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_recent_file_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_THUMBNAIL_PATH:
      priv->thumbnail_path = g_value_dup_string (value);
      break;
    case PROP_INFO:
      priv->info = (GtkRecentInfo *)g_value_get_pointer (value);
      gtk_recent_info_ref (priv->info);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_recent_file_tile_dispose (GObject *object)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  if (priv->info)
  {
    gtk_recent_info_unref (priv->info);
    priv->info = NULL;
  }

  if (priv->timeline)
  {
    g_object_unref (priv->timeline);
    priv->timeline = NULL;
  }

  if (priv->behave)
  {
    g_object_unref (priv->behave);
    priv->behave = NULL;
  }

  G_OBJECT_CLASS (penge_recent_file_tile_parent_class)->dispose (object);
}

static void
penge_recent_file_tile_finalize (GObject *object)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  g_free (priv->thumbnail_path);

  G_OBJECT_CLASS (penge_recent_file_tile_parent_class)->finalize (object);
}

static gboolean
_button_press_event (ClutterActor *actor,
                     ClutterEvent *event,
                     gpointer      userdata)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (userdata);

  if (!penge_utils_launch_for_uri (actor,
                                   gtk_recent_info_get_uri (priv->info)))
  {
    g_warning (G_STRLOC ": Error launching: %s",
               gtk_recent_info_get_uri (priv->info));
  } else {
    penge_utils_signal_activated (actor);
  }

  return TRUE;
}

static void
penge_recent_file_tile_constructed (GObject *object)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);
  ClutterActor *tex;
  GError *error = NULL;
  GFile *file;
  const gchar *content_type;
  gchar *type_description;
  const gchar *uri;
  GFileInfo *info;

  tex = g_object_new (PENGE_TYPE_MAGIC_TEXTURE,
                      NULL);

  if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (tex),
                                      priv->thumbnail_path,
                                      &error))
  {
    g_warning (G_STRLOC ": Error opening thumbnail: %s",
               error->message);
    g_clear_error (&error);
  } else {
    nbtk_table_add_actor (NBTK_TABLE (object),
                          tex,
                          0,
                          0);
    clutter_container_child_set (CLUTTER_CONTAINER (object),
                                 tex,
                                 "row-span",
                                 2,
                                 NULL);

    uri = gtk_recent_info_get_uri (priv->info);

    if (g_str_has_prefix (uri, "file:/"))
    {
      file = g_file_new_for_uri (uri);
      info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME
                                ","
                                G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                G_FILE_QUERY_INFO_NONE,
                                NULL,
                                &error);

      if (!info)
      {
        g_warning (G_STRLOC ": Error getting file info: %s",
                   error->message);
        g_clear_error (&error);
      } else {
        nbtk_label_set_text (NBTK_LABEL (priv->details_filename_label),
                             g_file_info_get_display_name (info));

        content_type = g_file_info_get_content_type (info);
        type_description = g_content_type_get_description (content_type);
        nbtk_label_set_text (NBTK_LABEL (priv->details_type_label),
                             type_description);
        g_free (type_description);
      }

      g_object_unref (info);
      g_object_unref (file);
    } else {
      nbtk_label_set_text (NBTK_LABEL (priv->details_filename_label),
                           gtk_recent_info_get_display_name (priv->info));
      if (g_str_has_prefix (uri, "http"))
      {
        nbtk_label_set_text (NBTK_LABEL (priv->details_type_label),
                             _("Web page"));
      } else {
        nbtk_label_set_text (NBTK_LABEL (priv->details_type_label),
                             "");
      }
    }

    /* Do this afterwards so that is is on top of the image */
    nbtk_table_add_actor (NBTK_TABLE (object),
                          (ClutterActor *)priv->details_overlay,
                          1,
                          0);
    clutter_container_child_set (CLUTTER_CONTAINER (object),
                                 (ClutterActor *)priv->details_overlay,
                                 "y-expand",
                                 FALSE,
                                 NULL);
  }

  g_signal_connect (object,
                    "button-press-event",
                    (GCallback)_button_press_event,
                    object);
}

static void
penge_recent_file_tile_class_init (PengeRecentFileTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeRecentFileTilePrivate));

  object_class->get_property = penge_recent_file_tile_get_property;
  object_class->set_property = penge_recent_file_tile_set_property;
  object_class->dispose = penge_recent_file_tile_dispose;
  object_class->finalize = penge_recent_file_tile_finalize;
  object_class->constructed = penge_recent_file_tile_constructed;

  pspec = g_param_spec_string ("thumbnail-path",
                               "Thumbnail path",
                               "Path to the thumbnail to use to represent"
                               "this recent file",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_THUMBNAIL_PATH, pspec);

  pspec = g_param_spec_pointer ("info",
                                "Recent file information",
                                "The GtkRecentInfo structure for this recent"
                                "file",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_INFO, pspec);
}

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (actor);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      "hover");

  clutter_timeline_set_direction (priv->timeline,
                                  CLUTTER_TIMELINE_FORWARD);
  if (!clutter_timeline_is_playing (priv->timeline))
  {
    clutter_timeline_rewind (priv->timeline);
    clutter_timeline_start (priv->timeline);
  }

  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (actor);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      NULL);

  clutter_timeline_set_direction (priv->timeline,
                                  CLUTTER_TIMELINE_BACKWARD);
  if (!clutter_timeline_is_playing (priv->timeline))
    clutter_timeline_start (priv->timeline);

  return FALSE;
}

static void
penge_recent_file_tile_init (PengeRecentFileTile *self)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (self);
  ClutterAlpha *alpha;
  ClutterActor *tmp_text;

  g_signal_connect (self,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    NULL);
  g_signal_connect (self,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    NULL);

  priv->details_overlay = nbtk_table_new ();
  clutter_actor_set_opacity ((ClutterActor *)priv->details_overlay,
                             0x0);

  priv->details_filename_label = nbtk_label_new ("Filename");
  tmp_text =
    nbtk_label_get_clutter_text (NBTK_LABEL (priv->details_filename_label));
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                   PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_END);

  nbtk_table_add_actor (NBTK_TABLE (priv->details_overlay),
                        (ClutterActor *)priv->details_filename_label,
                        0,
                        0);

  priv->details_type_label = nbtk_label_new ("Type");
  tmp_text =
    nbtk_label_get_clutter_text (NBTK_LABEL (priv->details_type_label));
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                              PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_END);


  nbtk_table_add_actor (NBTK_TABLE (priv->details_overlay),
                        (ClutterActor *)priv->details_type_label,
                        1,
                        0);

  nbtk_widget_set_style_class_name (priv->details_overlay,
                                    "PengeRecentFileTileDetails");
  nbtk_widget_set_style_class_name (priv->details_filename_label,
                                    "PengeRecentFileTileDetailsFilename");
  nbtk_widget_set_style_class_name (priv->details_type_label,
                                    "PengeRecentFileTileDetailsType");

  /* Animation for fading it in and out */
  /* TODO: Use ClutterAnimation */
  priv->timeline = clutter_timeline_new (300);

  alpha = clutter_alpha_new_full (priv->timeline,
                                  CLUTTER_LINEAR);
  priv->behave = clutter_behaviour_opacity_new (alpha, 0x00, 0xc0);
  clutter_behaviour_apply (priv->behave,
                           (ClutterActor *)priv->details_overlay);

  clutter_actor_set_reactive ((ClutterActor *)self, TRUE);
}


const gchar *
penge_recent_file_tile_get_uri (PengeRecentFileTile *tile)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (tile);

  return gtk_recent_info_get_uri (priv->info);
}


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

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <zeitgeist.h>

#include "penge-recent-file-tile.h"
#include "penge-magic-texture.h"
#include "penge-utils.h"


G_DEFINE_TYPE (PengeRecentFileTile, penge_recent_file_tile, PENGE_TYPE_INTERESTING_TILE)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_RECENT_FILE_TILE, PengeRecentFileTilePrivate))

#define GET_PRIVATE(o) ((PengeRecentFileTile *)o)->priv


struct _PengeRecentFileTilePrivate {
  gchar *thumbnail_path;
  ZeitgeistEvent *event;
  ClutterActor *tex;
};

enum
{
  PROP_0,
  PROP_THUMBNAIL_PATH,
  PROP_ZG_EVENT,
};

static void penge_recent_file_tile_update (PengeRecentFileTile *tile);
static void penge_recent_file_tile_update_thumbnail (PengeRecentFileTile *tile);

static void
penge_recent_file_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_THUMBNAIL_PATH:
      g_value_set_string (value, priv->thumbnail_path);
      break;
    case PROP_ZG_EVENT:
      g_value_set_object (value, priv->event);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_recent_file_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeRecentFileTile *tile = (PengeRecentFileTile *)object;
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);
  ZeitgeistEvent *event = NULL;

  switch (property_id) {
    case PROP_THUMBNAIL_PATH:
      if (priv->thumbnail_path)
        g_free (priv->thumbnail_path);

      priv->thumbnail_path = g_value_dup_string (value);
      penge_recent_file_tile_update_thumbnail (tile);
      break;
    case PROP_ZG_EVENT:
      event = (ZeitgeistEvent*) g_value_get_object (value);
      if (event == priv->event)
        return;

      if (priv->event)
        g_object_unref (priv->event);

      priv->event = event;

      if (event)
        g_object_ref (event);
      if (event)

      penge_recent_file_tile_update (tile);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_recent_file_tile_dispose (GObject *object)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (object);

  if (priv->event)
  {
    g_object_unref (priv->event);
    priv->event = NULL;
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

static void
_clicked_cb (MxButton *button,
             gpointer  userdata)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (userdata);
  ZeitgeistSubject *subj;

  /* FIXME we still assume there is only one subj per event */
  subj = zeitgeist_event_get_subject (priv->event, 0);

  if (!penge_utils_launch_for_uri ((ClutterActor *)button,
        zeitgeist_subject_get_uri (subj)))
  {
    g_warning (G_STRLOC ": Error launching: %s",
               zeitgeist_subject_get_uri (subj));
               
  } else {
    penge_utils_signal_activated ((ClutterActor *)button);
  }
}

static void
penge_recent_file_tile_update_thumbnail (PengeRecentFileTile *tile)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (tile);
  GError *error = NULL;

  if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->tex),
                                      priv->thumbnail_path,
                                      &error))
  {
    g_warning (G_STRLOC ": Error opening thumbnail: %s",
               error->message);
    g_clear_error (&error);
  }
}

static void
penge_recent_file_tile_update (PengeRecentFileTile *tile)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (tile);
  ZeitgeistSubject *subj;
  GError *error = NULL;
  GFile *file;
  const gchar *content_type;
  gchar *type_description;
  const gchar *uri;
  GFileInfo *info;

  subj = zeitgeist_event_get_subject (priv->event, 0);
  uri = zeitgeist_subject_get_uri (subj);

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
      content_type = g_file_info_get_content_type (info);
      type_description = g_content_type_get_description (content_type);
      g_object_set (tile,
                    "primary-text", g_file_info_get_display_name (info),
                    "secondary-text", type_description,
                    NULL);
      g_free (type_description);
    }

    g_object_unref (info);
    g_object_unref (file);
  } else {
    ZeitgeistSubject *s = zeitgeist_event_get_subject (priv->event, 0);

    if (g_str_has_prefix (uri, "http"))
    {

      g_object_set (tile,
                    "primary-text",
                    zeitgeist_subject_get_text (s),
                    "secondary-text",
                    _("Web page"),
                    NULL);
    } else {
      g_object_set (tile,
                    "primary-text",
                    zeitgeist_subject_get_text (s),
                    "secondary-text",
                    "",
                    NULL);
    }
  }
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

  pspec = g_param_spec_string ("thumbnail-path",
                               "Thumbnail path",
                               "Path to the thumbnail to use to represent"
                               "this recent file",
                               NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_THUMBNAIL_PATH, pspec);

  pspec = g_param_spec_object ("zg-event",
                              "Recent file event",
                              "The ZeitgeistEvent instance for this recent"
                              "file",
                              ZEITGEIST_TYPE_EVENT,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ZG_EVENT, pspec);
}

static void
penge_recent_file_tile_init (PengeRecentFileTile *self)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE_REAL (self);

  self->priv = priv;

  priv->tex = g_object_new (PENGE_TYPE_MAGIC_TEXTURE, NULL);

  g_object_set (self,
                "body", priv->tex,
                NULL);

  g_signal_connect (self,
                    "clicked",
                    (GCallback)_clicked_cb,
                    self);

  clutter_actor_set_reactive ((ClutterActor *)self, TRUE);
}


const gchar *
penge_recent_file_tile_get_uri (PengeRecentFileTile *tile)
{
  PengeRecentFileTilePrivate *priv = GET_PRIVATE (tile);
  ZeitgeistSubject *s = zeitgeist_event_get_subject (priv->event, 0);

  return zeitgeist_subject_get_uri (s);
}


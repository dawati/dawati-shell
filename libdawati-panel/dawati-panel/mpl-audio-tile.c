/*
 * Copyright (c) 2011 Intel Corporation.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Author: Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>
 */

#include "mpl-audio-tile.h"

#define DEFAULT_COVER_WIDTH (64)
#define DEFAULT_COVER_HEIGHT (64)

G_DEFINE_TYPE (MplAudioTile, mpl_audio_tile, MX_TYPE_BIN)

enum
{
  PROP_0,

  PROP_COVER_WIDTH,
  PROP_COVER_HEIGHT,

  PROP_URI,
  PROP_COVER_ART,
  PROP_SONG_TITLE,
  PROP_ARTIST_NAME,
  PROP_ALBUM_TITLE
};

#define MPL_AUDIO_TILE_PRIVATE(o)                                       \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                                    \
                                MPL_TYPE_AUDIO_TILE,                    \
                                MplAudioTilePrivate))

struct _MplAudioTilePrivate
{
  gint cover_width;
  gint cover_height;

  gchar *uri;
  gchar *cover_art_path;

  ClutterActor *cover_art;
  ClutterActor *song_title;
  ClutterActor *artist_name;
  ClutterActor *album_title;
};

static void
mpl_audio_tile_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  MplAudioTile        *self = MPL_AUDIO_TILE (object);
  MplAudioTilePrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_COVER_WIDTH:
      g_value_set_int (value, priv->cover_width);
      break;

    case PROP_COVER_HEIGHT:
      g_value_set_int (value, priv->cover_height);
      break;

    case PROP_URI:
      g_value_set_string (value, priv->uri);
      break;

    case PROP_COVER_ART:
      g_value_set_string (value, priv->cover_art_path);
      break;

    case PROP_SONG_TITLE:
      g_value_set_string (value, mx_label_get_text (MX_LABEL (priv->song_title)));
      break;

    case PROP_ARTIST_NAME:
      g_value_set_string (value, mx_label_get_text (MX_LABEL (priv->artist_name)));
      break;

    case PROP_ALBUM_TITLE:
      g_value_set_string (value, mx_label_get_text (MX_LABEL (priv->album_title)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_audio_tile_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  MplAudioTile        *self = MPL_AUDIO_TILE (object);
  MplAudioTilePrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_COVER_WIDTH:
      priv->cover_width = g_value_get_int (value);
      clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
      break;

    case PROP_COVER_HEIGHT:
      priv->cover_height = g_value_get_int (value);
      clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
      break;

    case PROP_URI:
      g_free (priv->uri);
      priv->uri = g_value_dup_string (value);
      break;

    case PROP_COVER_ART:
      g_free (priv->cover_art_path);
      priv->cover_art_path = g_value_dup_string (value);
      mx_image_set_from_file_at_size (MX_IMAGE (priv->cover_art),
                                      priv->cover_art_path,
                                      64, 64, NULL);
      break;

    case PROP_SONG_TITLE:
      mx_label_set_text (MX_LABEL (priv->song_title),
                         g_value_get_string (value));
      break;

    case PROP_ARTIST_NAME:
      mx_label_set_text (MX_LABEL (priv->artist_name),
                         g_value_get_string (value));
      break;

    case PROP_ALBUM_TITLE:
      mx_label_set_text (MX_LABEL (priv->album_title),
                         g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_audio_tile_dispose (GObject *object)
{
  MplAudioTile        *self = MPL_AUDIO_TILE (object);
  MplAudioTilePrivate *priv = self->priv;

  g_free (priv->cover_art_path);
  priv->cover_art_path = NULL;

  g_free (priv->uri);
  priv->uri = NULL;

  G_OBJECT_CLASS (mpl_audio_tile_parent_class)->dispose (object);
}

static void
mpl_audio_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (mpl_audio_tile_parent_class)->finalize (object);
}

static void
mpl_audio_tile_allocate (ClutterActor           *actor,
                         const ClutterActorBox  *box,
                         ClutterAllocationFlags  flags)
{
  MplAudioTilePrivate *priv = ((MplAudioTile *) actor)->priv;
  MxPadding padding;
  ClutterActorBox child_box;
  gfloat available_width, available_height;
  gfloat min, nat;

  CLUTTER_ACTOR_CLASS (mpl_audio_tile_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  available_width = box->x2 - box->x1 - padding.left - padding.right;
  available_height = box->y2 - box->y1 - padding.top - padding.bottom;

  child_box.x1 = padding.left;
  child_box.y1 = padding.top;

  /* cover */
  clutter_actor_get_preferred_width (priv->cover_art, -1, &min, &nat);
  child_box.x2 = MIN (child_box.x1 + nat, available_width);
  clutter_actor_get_preferred_height (priv->cover_art, -1, &min, &nat);
  child_box.y2 = MIN (child_box.y1 + nat, available_height);
  clutter_actor_allocate (priv->cover_art, &child_box, flags);

  child_box.x1 = child_box.x2;
  child_box.x2 = available_width;

  /* song title */
  clutter_actor_get_preferred_height (priv->song_title, -1, &min, &nat);
  child_box.y2 = MIN (child_box.y1 + nat, available_height);
  clutter_actor_allocate (priv->song_title, &child_box, flags);

  /* artist name */
  clutter_actor_get_preferred_height (priv->artist_name, -1, &min, &nat);
  child_box.y1 = child_box.y2;
  child_box.y2 = MIN (child_box.y1 + nat, available_height);
  clutter_actor_allocate (priv->artist_name, &child_box, flags);

  /* album title */
  clutter_actor_get_preferred_height (priv->album_title, -1, &min, &nat);
  child_box.y1 = child_box.y2;
  child_box.y2 = MIN (child_box.y1 + nat, available_height);
  clutter_actor_allocate (priv->album_title, &child_box, flags);
}

static void
mpl_audio_tile_paint (ClutterActor *actor)
{
  MplAudioTilePrivate *priv = ((MplAudioTile *)actor)->priv;

  clutter_actor_paint (priv->cover_art);
  clutter_actor_paint (priv->song_title);
  clutter_actor_paint (priv->artist_name);
  clutter_actor_paint (priv->album_title);
}

static void
mpl_audio_tile_class_init (MplAudioTileClass *klass)
{
  GParamSpec        *pspec;
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MplAudioTilePrivate));

  object_class->get_property = mpl_audio_tile_get_property;
  object_class->set_property = mpl_audio_tile_set_property;
  object_class->dispose      = mpl_audio_tile_dispose;
  object_class->finalize     = mpl_audio_tile_finalize;

  actor_class->allocate             = mpl_audio_tile_allocate;
  actor_class->paint                = mpl_audio_tile_paint;

  pspec = g_param_spec_int ("tile-width",
                            "Tile width",
                            "Tile width",
                            0, G_MAXINT,
                            DEFAULT_COVER_WIDTH,
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_COVER_WIDTH, pspec);

  pspec = g_param_spec_int ("tile-height",
                            "Tile height",
                            "Tile height",
                            0, G_MAXINT,
                            DEFAULT_COVER_HEIGHT,
                            G_PARAM_READWRITE |
                            G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_COVER_HEIGHT, pspec);

  pspec = g_param_spec_string ("uri",
                               "URI",
                               "URI",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_URI, pspec);

  pspec = g_param_spec_string ("cover-art",
                               "Cover art",
                               "Small picture for the item",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_COVER_ART, pspec);

  pspec = g_param_spec_string ("song-title",
                               "Song title",
                               "Song title",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SONG_TITLE, pspec);

  pspec = g_param_spec_string ("artist-name",
                               "Artist name",
                               "Artist name",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ARTIST_NAME, pspec);

  pspec = g_param_spec_string ("album-title",
                               "Album title",
                               "Album title",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ALBUM_TITLE, pspec);
}

static void
mpl_audio_tile_init (MplAudioTile *self)
{
  ClutterActor *actor = CLUTTER_ACTOR (self);
  MplAudioTilePrivate *priv = self->priv = MPL_AUDIO_TILE_PRIVATE (self);

  priv->cover_width = DEFAULT_COVER_WIDTH;
  priv->cover_height = DEFAULT_COVER_HEIGHT;

  priv->cover_art = mx_image_new ();
  clutter_actor_set_name (priv->cover_art, "dawati-audio-tile-cover");
  mx_image_set_from_cogl_texture (MX_IMAGE (priv->cover_art),
                                  cogl_texture_new_with_size (1, 1, COGL_TEXTURE_NO_SLICING,
                                                              COGL_PIXEL_FORMAT_RGBA_8888_PRE));
  clutter_actor_add_child (actor, priv->cover_art);

  priv->song_title = mx_label_new ();
  clutter_actor_add_child (actor, priv->song_title);
  clutter_actor_set_name (priv->song_title, "dawati-audio-tile-title");

  priv->artist_name = mx_label_new ();
  clutter_actor_add_child (actor, priv->artist_name);
  clutter_actor_set_name (priv->song_title, "dawati-audio-tile-artist");

  priv->album_title = mx_label_new ();
  clutter_actor_add_child (actor, priv->album_title);
  clutter_actor_set_name (priv->song_title, "dawati-audio-tile-album");
}

/**
 * mpl_audio_tile_new:
 *
 * Create a new #MplAudioTile
 *
 * Returns: a new #MplAudioTile
 */
ClutterActor *
mpl_audio_tile_new (void)
{
  return g_object_new (MPL_TYPE_AUDIO_TILE, NULL);
}

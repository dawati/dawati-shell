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

#include "mpl-audio-item.h"

G_DEFINE_TYPE (MplAudioItem, mpl_audio_item, MX_TYPE_BIN)

enum
{
  PROP_0,

  PROP_MAX_WIDTH,

  PROP_URI,
  PROP_COVER_ART,
  PROP_SONG_TITLE,
  PROP_ARTIST_NAME,
  PROP_ALBUM_TITLE
};

#define MPL_AUDIO_ITEM_PRIVATE(o)                                       \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),                                    \
                                MPL_TYPE_AUDIO_ITEM,                    \
                                MplAudioItemPrivate))

struct _MplAudioItemPrivate
{
  guint max_width;

  gchar *uri;
  gchar *cover_art_path;

  ClutterActor *cover_art;
  ClutterActor *song_title;
  ClutterActor *artist_name;
  ClutterActor *album_title;
};

static void
mpl_audio_item_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  MplAudioItem        *self = MPL_AUDIO_ITEM (object);
  MplAudioItemPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_MAX_WIDTH:
      g_value_set_uint (value, priv->max_width);
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
mpl_audio_item_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  MplAudioItem        *self = MPL_AUDIO_ITEM (object);
  MplAudioItemPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_MAX_WIDTH:
      priv->max_width = g_value_get_uint (value);
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
mpl_audio_item_dispose (GObject *object)
{
  MplAudioItem        *self = MPL_AUDIO_ITEM (object);
  MplAudioItemPrivate *priv = self->priv;

  g_free (priv->cover_art_path);
  priv->cover_art_path = NULL;

  g_free (priv->uri);
  priv->uri = NULL;

  if (priv->cover_art)
    {
      clutter_actor_destroy (priv->cover_art);
      priv->cover_art = NULL;
    }

  if (priv->song_title)
    {
      clutter_actor_destroy (priv->song_title);
      priv->song_title = NULL;
    }

  if (priv->song_title)
    {
      clutter_actor_destroy (priv->song_title);
      priv->song_title = NULL;
    }

  if (priv->song_title)
    {
      clutter_actor_destroy (priv->song_title);
      priv->song_title = NULL;
    }

  G_OBJECT_CLASS (mpl_audio_item_parent_class)->dispose (object);
}

static void
mpl_audio_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (mpl_audio_item_parent_class)->finalize (object);
}

static void
mpl_audio_item_get_preferred_width (ClutterActor *actor,
                                    gfloat        for_height,
                                    gfloat       *min_width_p,
                                    gfloat       *nat_width_p)
{
  MplAudioItemPrivate *priv = ((MplAudioItem *) actor)->priv;
  MxPadding padding;
  gfloat min, nat;
  gfloat tmin, tnat;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  for_height -= padding.top + padding.bottom;

  tmin = tnat = padding.left + padding.right;

  clutter_actor_get_preferred_width (priv->cover_art, for_height, &min, &nat);
  tmin += min;
  tnat += nat;

  clutter_actor_get_preferred_width (priv->song_title, for_height, &min, &nat);
  tmin = MIN (tmin + min, priv->max_width);
  tnat = MIN (tnat + nat, priv->max_width);

  clutter_actor_get_preferred_width (priv->artist_name, for_height, &min, &nat);
  tmin = MIN (tmin + min, priv->max_width);
  tnat = MIN (tnat + nat, priv->max_width);

  clutter_actor_get_preferred_width (priv->album_title, for_height, &min, &nat);
  tmin = MIN (tmin + min, priv->max_width);
  tnat = MIN (tnat + nat, priv->max_width);

  if (min_width_p)
    *min_width_p = tmin;
  if (nat_width_p)
    *nat_width_p = tnat;
}

static void
mpl_audio_item_get_preferred_height (ClutterActor *actor,
                                     gfloat        for_width,
                                     gfloat       *min_height_p,
                                     gfloat       *nat_height_p)
{
  MplAudioItemPrivate *priv = ((MplAudioItem *) actor)->priv;
  MxPadding padding;
  gfloat min, nat, min2, nat2;
  gfloat tmin, tnat;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  for_width -= padding.left + padding.right;

  tmin = tnat = padding.top + padding.bottom;
  clutter_actor_get_preferred_height (priv->cover_art, for_width, &min, &nat);
  tmin += min;
  tnat += nat;

  min = nat = padding.top + padding.bottom;
  clutter_actor_get_preferred_height (priv->song_title, for_width, &min2, &nat2);
  min += min2;
  nat += nat2;

  clutter_actor_get_preferred_height (priv->artist_name, for_width, &min2, &nat2);
  min += min2;
  nat += nat2;

  clutter_actor_get_preferred_height (priv->album_title, for_width, &min2, &nat2);
  min += min2;
  nat += nat2;

  tmin = /* MIN ( */MAX (tmin, min)/* , priv->max_height) */;
  tnat = /* MIN ( */MAX (tnat, nat)/* , priv->max_height) */;

  if (min_height_p)
    *min_height_p = tmin;
  if (nat_height_p)
    *nat_height_p = tnat;
}


static void
mpl_audio_item_allocate (ClutterActor           *actor,
                         const ClutterActorBox  *box,
                         ClutterAllocationFlags  flags)
{
  MplAudioItemPrivate *priv = ((MplAudioItem *) actor)->priv;
  MxPadding padding;
  ClutterActorBox child_box;
  gfloat available_width, available_height;
  gfloat min, nat;

  CLUTTER_ACTOR_CLASS (mpl_audio_item_parent_class)->allocate (actor, box, flags);

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
mpl_audio_item_paint (ClutterActor *actor)
{
  MplAudioItemPrivate *priv = ((MplAudioItem *)actor)->priv;

  clutter_actor_paint (priv->cover_art);
  clutter_actor_paint (priv->song_title);
  clutter_actor_paint (priv->artist_name);
  clutter_actor_paint (priv->album_title);
}

static void
mpl_audio_item_class_init (MplAudioItemClass *klass)
{
  GParamSpec        *pspec;
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MplAudioItemPrivate));

  object_class->get_property = mpl_audio_item_get_property;
  object_class->set_property = mpl_audio_item_set_property;
  object_class->dispose      = mpl_audio_item_dispose;
  object_class->finalize     = mpl_audio_item_finalize;

  actor_class->get_preferred_height = mpl_audio_item_get_preferred_height;
  actor_class->get_preferred_width  = mpl_audio_item_get_preferred_width;
  actor_class->allocate             = mpl_audio_item_allocate;
  actor_class->paint                = mpl_audio_item_paint;

  pspec = g_param_spec_uint ("max-width",
                             "Max width",
                             "Maximum width",
                             0, G_MAXUINT, 200,
                             G_PARAM_READWRITE |
                             G_PARAM_CONSTRUCT |
                             G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_MAX_WIDTH, pspec);

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
mpl_audio_item_init (MplAudioItem *self)
{
  ClutterActor *actor = CLUTTER_ACTOR (self);
  MplAudioItemPrivate *priv = self->priv = MPL_AUDIO_ITEM_PRIVATE (self);

  priv->cover_art = mx_image_new ();
  clutter_actor_set_name (priv->cover_art, "dawati-audio-item-cover");
  mx_image_set_from_cogl_texture (MX_IMAGE (priv->cover_art),
                                  cogl_texture_new_with_size (1, 1, COGL_TEXTURE_NO_SLICING,
                                                              COGL_PIXEL_FORMAT_RGBA_8888_PRE));
  clutter_actor_set_parent (priv->cover_art, actor);

  priv->song_title = mx_label_new ();
  clutter_actor_set_parent (priv->song_title, actor);
  clutter_actor_set_name (priv->song_title, "dawati-audio-item-title");

  priv->artist_name = mx_label_new ();
  clutter_actor_set_parent (priv->artist_name, actor);
  clutter_actor_set_name (priv->song_title, "dawati-audio-item-artist");

  priv->album_title = mx_label_new ();
  clutter_actor_set_parent (priv->album_title, actor);
  clutter_actor_set_name (priv->song_title, "dawati-audio-item-album");
}

/**
 * mpl_audio_item_new:
 *
 * Create a new #MplAudioItem
 *
 * Returns: a new #MplAudioItem
 */
ClutterActor *
mpl_audio_item_new (void)
{
  return g_object_new (MPL_TYPE_AUDIO_ITEM, NULL);
}

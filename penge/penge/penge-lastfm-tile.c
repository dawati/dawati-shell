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


#include <gio/gio.h>
#include <mojito-client/mojito-item.h>

#include "penge-utils.h"
#include "penge-lastfm-tile.h"
#include "penge-magic-texture.h"

G_DEFINE_TYPE (PengeLastfmTile, penge_lastfm_tile, PENGE_TYPE_PEOPLE_TILE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_LASTFM_TILE, PengeLastfmTilePrivate))

#define DEFAULT_ALBUM_ARTWORK THEMEDIR "/default-album-artwork.png"

typedef struct _PengeLastfmTilePrivate PengeLastfmTilePrivate;

struct _PengeLastfmTilePrivate {
  MojitoItem *item;
};

enum
{
  PROP_0,
  PROP_ITEM
};

static void
penge_lastfm_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeLastfmTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_ITEM:
      g_value_set_pointer (value, priv->item);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_lastfm_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeLastfmTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_ITEM:
      priv->item = g_value_get_pointer (value);
      mojito_item_ref (priv->item);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_lastfm_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_lastfm_tile_parent_class)->finalize (object);
}

static gboolean
_button_press_event (ClutterActor *actor,
                     ClutterEvent *event,
                     gpointer      userdata)
{
  PengeLastfmTilePrivate *priv = GET_PRIVATE (userdata);

  penge_people_tile_activate ((PengePeopleTile *)actor,
                              priv->item);

  return TRUE;
}

static void
penge_lastfm_tile_constructed (GObject *object)
{
  PengeLastfmTilePrivate *priv = GET_PRIVATE (object);
  const gchar *title;
  const gchar *author;
  const gchar *thumbnail_path;
  const gchar *buddyicon_path;
  ClutterActor *body;
  GError *error = NULL;

  g_return_if_fail (priv->item != NULL);

  title = g_hash_table_lookup (priv->item->props,
                               "title");
  author = g_hash_table_lookup (priv->item->props,
                                "author");
  buddyicon_path = g_hash_table_lookup (priv->item->props,
                                        "authoricon");

  g_object_set (object,
                "primary-text",
                title,
                "secondary-text",
                author,
                "icon-path",
                buddyicon_path,
                NULL);

  thumbnail_path = g_hash_table_lookup (priv->item->props,
                                        "thumbnail");

  body = g_object_new (PENGE_TYPE_MAGIC_TEXTURE, NULL);

  if (!thumbnail_path)
  {
    thumbnail_path = DEFAULT_ALBUM_ARTWORK;
  }

  if (clutter_texture_set_from_file (CLUTTER_TEXTURE (body),
                                     thumbnail_path,
                                     &error))
  {
    g_object_set (object,
                  "body",
                  body,
                  NULL);
    g_object_unref (body);
  } else {
    g_critical (G_STRLOC ": Loading thumbnail failed: %s",
                error->message);
    g_clear_error (&error);
  }

  g_signal_connect (object,
                    "button-press-event",
                    (GCallback)_button_press_event,
                    object);
}

static void
penge_lastfm_tile_class_init (PengeLastfmTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeLastfmTilePrivate));

  object_class->get_property = penge_lastfm_tile_get_property;
  object_class->set_property = penge_lastfm_tile_set_property;
  object_class->finalize = penge_lastfm_tile_finalize;
  object_class->constructed = penge_lastfm_tile_constructed;

  pspec = g_param_spec_pointer ("item",
                                "Item",
                                "Client side item to render",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);
}

static void
penge_lastfm_tile_init (PengeLastfmTile *self)
{
}


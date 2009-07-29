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
#include <moblin-panel/mpl-utils.h>

#include "penge-utils.h"
#include "penge-flickr-tile.h"
#include "penge-magic-texture.h"

G_DEFINE_TYPE (PengeFlickrTile, penge_flickr_tile, PENGE_TYPE_PEOPLE_TILE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_FLICKR_TILE, PengeFlickrTilePrivate))

typedef struct _PengeFlickrTilePrivate PengeFlickrTilePrivate;

struct _PengeFlickrTilePrivate {
    MojitoItem *item;
};

enum
{
  PROP_0,
  PROP_ITEM
};

static void
penge_flickr_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeFlickrTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_ITEM:
      g_value_set_pointer (value, priv->item);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_flickr_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeFlickrTilePrivate *priv = GET_PRIVATE (object);

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
penge_flickr_tile_dispose (GObject *object)
{
  PengeFlickrTilePrivate *priv = GET_PRIVATE (object);

  if (priv->item)
  {
    mojito_item_unref (priv->item);
    priv->item = NULL;
  }

  G_OBJECT_CLASS (penge_flickr_tile_parent_class)->dispose (object);
}

static void
penge_flickr_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_flickr_tile_parent_class)->finalize (object);
}

static gboolean
_button_press_event (ClutterActor *actor,
                     ClutterEvent *event,
                     gpointer      userdata)
{
  PengeFlickrTilePrivate *priv = GET_PRIVATE (userdata);

  penge_people_tile_activate ((PengePeopleTile *)actor,
                              priv->item);

  return TRUE;
}

static void
penge_flickr_tile_constructed (GObject *object)
{
  PengeFlickrTile *tile = PENGE_FLICKR_TILE (object);
  PengeFlickrTilePrivate *priv = GET_PRIVATE (tile);
  const gchar *title;
  const gchar *author;
  gchar *date;
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
  thumbnail_path = g_hash_table_lookup (priv->item->props,
                                        "thumbnail");
  date = mpl_utils_format_time (&(priv->item->date));
  g_object_set (tile,
                "primary-text",
                title,
                "secondary-text",
                author,
                "icon-path",
                buddyicon_path,
                NULL);
  g_free (date);

  body = g_object_new (PENGE_TYPE_MAGIC_TEXTURE, NULL);

  if (clutter_texture_set_from_file (CLUTTER_TEXTURE (body),
                                     thumbnail_path, 
                                     &error))
  {
    g_object_set (tile,
                  "body",
                  body,
                  NULL);
    g_object_unref (body);
  } else {
    g_critical (G_STRLOC ": Loading thumbnail failed: %s",
                error->message);
    g_clear_error (&error);
  }

  g_signal_connect (tile, 
                    "button-press-event",
                   (GCallback)_button_press_event,
                   tile);
}

static void
penge_flickr_tile_class_init (PengeFlickrTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeFlickrTilePrivate));

  object_class->get_property = penge_flickr_tile_get_property;
  object_class->set_property = penge_flickr_tile_set_property;
  object_class->dispose = penge_flickr_tile_dispose;
  object_class->finalize = penge_flickr_tile_finalize;
  object_class->constructed = penge_flickr_tile_constructed;

  pspec = g_param_spec_pointer ("item",
                                "Item",
                                "Client side item to render",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);
}

static void
penge_flickr_tile_init (PengeFlickrTile *self)
{

}



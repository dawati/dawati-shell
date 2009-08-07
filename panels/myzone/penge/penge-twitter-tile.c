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


#include <mojito-client/mojito-item.h>
#include <nbtk/nbtk.h>
#include <gio/gio.h>
#include <moblin-panel/mpl-utils.h>

#include "penge-twitter-tile.h"
#include "penge-utils.h"

G_DEFINE_TYPE (PengeTwitterTile, penge_twitter_tile, PENGE_TYPE_PEOPLE_TILE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_TWITTER_TILE, PengeTwitterTilePrivate))

typedef struct _PengeTwitterTilePrivate PengeTwitterTilePrivate;

struct _PengeTwitterTilePrivate {
    MojitoItem *item;
};

enum
{
  PROP_0,
  PROP_ITEM
};

static void
penge_twitter_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeTwitterTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_ITEM:
      g_value_set_pointer (value, priv->item);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_twitter_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeTwitterTilePrivate *priv = GET_PRIVATE (object);

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
penge_twitter_tile_dispose (GObject *object)
{
  PengeTwitterTilePrivate *priv = GET_PRIVATE (object);

  if (priv->item)
  {
    mojito_item_unref (priv->item);
    priv->item = NULL;
  }

  G_OBJECT_CLASS (penge_twitter_tile_parent_class)->dispose (object);
}

static void
penge_twitter_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_twitter_tile_parent_class)->finalize (object);
}


static gboolean
_button_press_event (ClutterActor *actor,
                     ClutterEvent *event,
                     gpointer      userdata)
{
  PengeTwitterTilePrivate *priv = GET_PRIVATE (userdata);

  penge_people_tile_activate ((PengePeopleTile *)actor,
                              priv->item);

  return TRUE;
}

static void
penge_twitter_tile_constructed (GObject *object)
{
  PengeTwitterTile *tile = PENGE_TWITTER_TILE (object);
  PengeTwitterTilePrivate *priv = GET_PRIVATE (tile);
  const gchar *content;
  const gchar *author;
  const gchar *authoricon_path;
  gchar *date;
  NbtkWidget *body;
  NbtkWidget *label;
  ClutterActor *tmp_text;

  g_return_if_fail (priv->item != NULL);

  content = g_hash_table_lookup (priv->item->props,
                                 "content");
  author = g_hash_table_lookup (priv->item->props,
                                "author");
  authoricon_path = g_hash_table_lookup (priv->item->props,
                                         "authoricon");

  date = mpl_utils_format_time (&(priv->item->date));

  g_object_set (tile,
                "primary-text",
                author,
                "secondary-text",
                date,
                "icon-path",
                authoricon_path,
                NULL);
  g_free (date);

  body = nbtk_bin_new ();
  nbtk_widget_set_style_class_name (body,
                                    "PengeTwitterTileBackground");
  label = nbtk_label_new (content);
  nbtk_widget_set_style_class_name (label, "PengeTwitterTileLabel");
  nbtk_bin_set_child (NBTK_BIN (body), (ClutterActor *)label);
  nbtk_bin_set_alignment (NBTK_BIN (body), NBTK_ALIGN_TOP, NBTK_ALIGN_TOP);
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                   PANGO_ALIGN_LEFT);

  g_object_set (tile,
                "body",
                body,
                NULL);

  g_signal_connect (tile,
                    "button-press-event",
                   (GCallback)_button_press_event,
                   tile);
}

static void
penge_twitter_tile_class_init (PengeTwitterTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeTwitterTilePrivate));

  object_class->get_property = penge_twitter_tile_get_property;
  object_class->set_property = penge_twitter_tile_set_property;
  object_class->dispose = penge_twitter_tile_dispose;
  object_class->finalize = penge_twitter_tile_finalize;
  object_class->constructed = penge_twitter_tile_constructed;

  pspec = g_param_spec_pointer ("item",
                                "Item",
                                "Client side item to render",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);
}

static void
penge_twitter_tile_init (PengeTwitterTile *self)
{
}


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

#include <meego-panel/mpl-utils.h>
#include <libsocialweb-client/sw-client.h>

#include "penge-people-tile.h"
#include "penge-utils.h"
#include "penge-magic-texture.h"
#include "penge-clickable-label.h"

G_DEFINE_TYPE (PengePeopleTile, penge_people_tile, PENGE_TYPE_INTERESTING_TILE)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_TILE, PengePeopleTilePrivate))


#define GET_PRIVATE(o) ((PengePeopleTile *)o)->priv

struct _PengePeopleTilePrivate {
  SwItem *item;
};

enum
{
  PROP_0,
  PROP_ITEM
};

#define DEFAULT_AVATAR_PATH THEMEDIR "/avatar_icon.png"
#define DEFAULT_ALBUM_ARTWORK THEMEDIR "/default-album-artwork.png"

static void
penge_people_tile_set_item (PengePeopleTile *tile,
                            SwItem          *item);

static void
penge_people_tile_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_ITEM:
      g_value_set_boxed (value, priv->item);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_people_tile_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  switch (property_id) {
    case PROP_ITEM:
      penge_people_tile_set_item ((PengePeopleTile *)object,
                                  g_value_get_boxed (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_people_tile_dispose (GObject *object)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (object);

  if (priv->item)
  {
    sw_item_unref (priv->item);
    priv->item = NULL;
  }

  G_OBJECT_CLASS (penge_people_tile_parent_class)->dispose (object);
}

static void
penge_people_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_tile_parent_class)->finalize (object);
}

static void
penge_people_tile_class_init (PengePeopleTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengePeopleTilePrivate));

  object_class->get_property = penge_people_tile_get_property;
  object_class->set_property = penge_people_tile_set_property;
  object_class->dispose = penge_people_tile_dispose;
  object_class->finalize = penge_people_tile_finalize;

  pspec = g_param_spec_boxed ("item",
                              "Item",
                              "The item to show",
                              SW_TYPE_ITEM,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);
}

static void
_clicked_cb (MxButton *button,
             gpointer  userdata)
{
  PengePeopleTile *tile = PENGE_PEOPLE_TILE (button);
  PengePeopleTilePrivate *priv = GET_PRIVATE (button);

  penge_people_tile_activate (tile, priv->item);
}

static void
penge_people_tile_init (PengePeopleTile *self)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE_REAL (self);

  self->priv = priv;

  g_signal_connect (self,
                    "clicked",
                    (GCallback)_clicked_cb,
                    self);
}

void
penge_people_tile_activate (PengePeopleTile *tile,
                            SwItem          *item)
{
  const gchar *url;

  url = g_hash_table_lookup (item->props,
                             "url");

  if (!penge_utils_launch_for_uri ((ClutterActor *)tile, url))
  {
    g_warning (G_STRLOC ": Error launching uri: %s",
               url);
  } else {
    penge_utils_signal_activated ((ClutterActor *)tile);
  }
}

static void
_label_url_clicked_cb (PengeClickableLabel *label,
                       const gchar         *url,
                       PengePeopleTile     *tile)
{
  if (!penge_utils_launch_for_uri ((ClutterActor *)tile, url))
  {
    g_warning (G_STRLOC ": Error launching uri: %s",
               url);
  } else {
    penge_utils_signal_activated ((ClutterActor *)tile);
  }
}

static void
penge_people_tile_set_text (PengePeopleTile *tile)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (tile);
  gchar *date;

  if (sw_item_has_key (priv->item, "title"))
  {
    if (sw_item_has_key (priv->item, "author"))
    {
      g_object_set (tile,
                    "primary-text", sw_item_get_value (priv->item, "title"),
                    "secondary-text", sw_item_get_value (priv->item, "author"),
                    NULL);
    } else {
      date = mx_utils_format_time (&(priv->item->date));
      g_object_set (tile,
                    "primary-text", sw_item_get_value (priv->item, "title"),
                    "secondary-text", date,
                    NULL);
      g_free (date);
    }
  } else if (sw_item_has_key (priv->item, "author")) {
      date = mx_utils_format_time (&(priv->item->date));
      g_object_set (tile,
                    "primary-text", sw_item_get_value (priv->item, "author"),
                    "secondary-text", date,
                    NULL);
      g_free (date);
  } else {
    g_assert_not_reached ();
  }
}

static void
penge_people_tile_set_item (PengePeopleTile *tile,
                            SwItem          *item)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (tile);
  ClutterActor *body, *tmp_text;
  ClutterActor *label;
  const gchar *content, *thumbnail;
  gchar *date;
  GError *error = NULL;
  ClutterActor *avatar = NULL;
  ClutterActor *avatar_bin;
  const gchar *author_icon;

  if (priv->item != item)
  {
    if (priv->item)
      sw_item_unref (priv->item);

    if (item)
      priv->item = sw_item_ref (item);
    else
      priv->item = NULL;
  }

  if (!priv->item)
    return;

  if (sw_item_has_key (item, "thumbnail"))
  {
    thumbnail = sw_item_get_value (item, "thumbnail");
    body = g_object_new (PENGE_TYPE_MAGIC_TEXTURE, NULL);

    if (clutter_texture_set_from_file (CLUTTER_TEXTURE (body),
                                       thumbnail,
                                       &error))
    {
      g_object_set (tile,
                    "body",
                    body,
                    NULL);
    } else {
      g_critical (G_STRLOC ": Loading thumbnail failed: %s",
                  error->message);
      g_clear_error (&error);
    }
  } else if (sw_item_has_key (item, "content")) {
    ClutterActor *inner_table;
    content = sw_item_get_value (item, "content");
    body = mx_frame_new ();
    inner_table = mx_table_new ();

    mx_table_set_column_spacing (MX_TABLE (inner_table), 6);
    mx_stylable_set_style_class (MX_STYLABLE (body),
                                 "PengePeopleTileContentBackground");
    mx_bin_set_child (MX_BIN (body), inner_table);
    mx_bin_set_fill (MX_BIN (body), TRUE, TRUE);
    author_icon = sw_item_get_value (item, "authoricon");
    mx_stylable_set_style_class (MX_STYLABLE (inner_table),
                                 "PengePeopleTileInnerTable");
    if (author_icon)
    {
      avatar = clutter_texture_new_from_file (author_icon, NULL);
    } else {
      avatar = clutter_texture_new_from_file (DEFAULT_AVATAR_PATH, NULL);
    }

    avatar_bin = mx_frame_new ();
    mx_bin_set_child (MX_BIN (avatar_bin), avatar);
    mx_bin_set_fill (MX_BIN (avatar_bin), TRUE, TRUE);
    mx_stylable_set_style_class (MX_STYLABLE (avatar_bin),
                                 "PengePeopleTileAvatarBackground");
    mx_table_add_actor_with_properties (MX_TABLE (inner_table),
                                        avatar_bin,
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", TRUE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_MIDDLE,
                                        "y-align", MX_ALIGN_START,
                                        NULL);
    clutter_actor_set_size (avatar, 48, 48);

    label = penge_clickable_label_new (content);
    g_signal_connect (label,
                      "url-clicked",
                      (GCallback)_label_url_clicked_cb,
                      tile);

    mx_stylable_set_style_class (MX_STYLABLE (label), "PengePeopleTileContentLabel");
    mx_table_add_actor_with_properties (MX_TABLE (inner_table),
                                        label,
                                        0, 1,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_START,
                                        NULL);

    tmp_text = mx_label_get_clutter_text (MX_LABEL (label));
    clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
    clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                     PANGO_WRAP_WORD_CHAR);
    clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                                PANGO_ELLIPSIZE_END);
    clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                     PANGO_ALIGN_LEFT);
    g_object_set (tile,
                  "body", body,
                  NULL);
  } else {
    if (g_str_equal (item->service, "lastfm"))
    {
      body = g_object_new (PENGE_TYPE_MAGIC_TEXTURE, NULL);

      if (clutter_texture_set_from_file (CLUTTER_TEXTURE (body),
                                         DEFAULT_ALBUM_ARTWORK,
                                         &error))
      {
        g_object_set (tile,
                      "body",
                      body,
                      NULL);
      } else {
        g_critical (G_STRLOC ": Loading thumbnail failed: %s",
                    error->message);
        g_clear_error (&error);
      }
    } else {
      g_assert_not_reached ();
    }
  }

  penge_people_tile_set_text (tile);

  if (!avatar)
  {
    author_icon = sw_item_get_value (item, "authoricon");

    if (author_icon)
    {
      g_object_set (tile,
                    "icon-path", author_icon,
                    NULL);
    } else {
      g_object_set (tile,
                    "icon-path", DEFAULT_AVATAR_PATH,
                    NULL);
    }
  } else {
    g_object_set (tile,
                  "icon-path", NULL,
                  NULL);
  }
}

void
penge_people_tile_refresh (PengePeopleTile *tile)
{
  penge_people_tile_set_text (tile);
}

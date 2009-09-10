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

#include <moblin-panel/mpl-utils.h>

#include "penge-people-tile.h"
#include "penge-utils.h"
#include "penge-magic-texture.h"

G_DEFINE_TYPE (PengePeopleTile, penge_people_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_TILE, PengePeopleTilePrivate))

typedef struct _PengePeopleTilePrivate PengePeopleTilePrivate;

struct _PengePeopleTilePrivate {
  ClutterActor *body;
  ClutterActor *icon;
  NbtkWidget *bin;
  NbtkWidget *primary_text;
  NbtkWidget *secondary_text;
  NbtkWidget *details_overlay;
  MojitoItem *item;
};

enum
{
  PROP_0,
  PROP_BODY,
  PROP_ICON_PATH,
  PROP_PRIMARY_TEXT,
  PROP_SECONDARY_TEXT,
  PROP_ITEM
};

#define DEFAULT_AVATAR_PATH THEMEDIR "/default-avatar-icon.png"
#define DEFAULT_ALBUM_ARTWORK THEMEDIR "/default-album-artwork.png"

static void
penge_people_tile_set_item (PengePeopleTile *tile,
                            MojitoItem      *item);

static void
penge_people_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
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
penge_people_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (object);
  GError *error = NULL;
  const gchar *path;

  switch (property_id) {
    case PROP_BODY:
      if (priv->body)
      {
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->bin),
                                        priv->body);
      }

      priv->body = g_value_get_object (value);

      if (!priv->body)
        return;

      nbtk_bin_set_child (NBTK_BIN (priv->bin),
                          priv->body);


      break;
    case PROP_ICON_PATH:

      path = g_value_get_string (value);

      if (!path)
      {
        path = DEFAULT_AVATAR_PATH;
      }

      if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon), 
                                          path,
                                          &error))
      {
        g_critical (G_STRLOC ": error setting icon texture from file: %s",
                    error->message);
        g_clear_error (&error);
      }

      break;
    case PROP_PRIMARY_TEXT:
      nbtk_label_set_text (NBTK_LABEL (priv->primary_text),
                           g_value_get_string (value));
      break;
    case PROP_SECONDARY_TEXT:
      nbtk_label_set_text (NBTK_LABEL (priv->secondary_text),
                           g_value_get_string (value));
      break;
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
    mojito_item_unref (priv->item);
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

  pspec = g_param_spec_object ("body",
                               "Body",
                               "Actor for the body of the tile",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_BODY, pspec);

  pspec = g_param_spec_string ("icon-path",
                               "Icon path",
                               "Path to icon to use in the tile",
                               NULL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_ICON_PATH, pspec);

  pspec = g_param_spec_string ("primary-text",
                               "Primary text",
                               "Primary text for the tile",
                               NULL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_PRIMARY_TEXT, pspec);

  pspec = g_param_spec_string ("secondary-text",
                               "Secondary text",
                               "Secondary text for the tile",
                               NULL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_SECONDARY_TEXT, pspec);

  pspec = g_param_spec_boxed ("item",
                              "Item",
                              "The item to show",
                              MOJITO_TYPE_ITEM,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);
}

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (userdata);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      "hover");

  if (1)
  {
    clutter_actor_animate (priv->details_overlay,
                           CLUTTER_LINEAR,
                           150,
                           "opacity", 0xc0,
                           NULL);
  } else {
    clutter_actor_animate (priv->details_overlay,
                           CLUTTER_LINEAR,
                           150,
                           "opacity", 0,
                           NULL);
  }

  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (userdata);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      NULL);

  if (0)
  {
    clutter_actor_animate (priv->details_overlay,
                           CLUTTER_LINEAR,
                           150,
                           "opacity", 0xc0,
                           NULL);
  } else {
    clutter_actor_animate (priv->details_overlay,
                           CLUTTER_LINEAR,
                           150,
                           "opacity", 0,
                           NULL);
  }

  return FALSE;
}

static void
penge_people_tile_init (PengePeopleTile *self)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_text;
  ClutterAlpha *alpha;

  priv->bin = nbtk_bin_new ();
  nbtk_bin_set_fill (NBTK_BIN (priv->bin), TRUE, TRUE);
  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->bin,
                        0,
                        0);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->bin,
                               "y-align", 0.0,
                               "x-align", 0.0,
                               "row-span", 2,
                               "y-fill", TRUE,
                               "y-expand", TRUE,
                               NULL);

  priv->primary_text = nbtk_label_new ("Primary text");
  nbtk_widget_set_style_class_name (priv->primary_text, 
                                    "PengePeopleTilePrimaryLabel");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->primary_text));
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                   PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), 
                              PANGO_ELLIPSIZE_END);

  priv->secondary_text = nbtk_label_new ("Secondary text");
  nbtk_widget_set_style_class_name (priv->secondary_text, 
                                    "PengePeopleTileSecondaryLabel");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->secondary_text));
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                   PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), 
                              PANGO_ELLIPSIZE_END);

  priv->icon = clutter_texture_new ();
  clutter_actor_set_size (priv->icon, 28, 28);

  /* This gets added to ourself table after our body because of ordering */
  priv->details_overlay = nbtk_table_new ();
  nbtk_widget_set_style_class_name (priv->details_overlay,
                                    "PengePeopleTileDetails");
  clutter_actor_set_opacity ((ClutterActor *)priv->details_overlay, 0x0);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->details_overlay,
                        1,
                        0);

  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->details_overlay,
                               "x-expand",
                               TRUE,
                               "y-expand",
                               TRUE,
                               "y-fill",
                               FALSE,
                               "y-align",
                               1.0,
                               NULL);

  nbtk_table_add_actor (NBTK_TABLE (priv->details_overlay),
                        (ClutterActor *)priv->primary_text,
                        0,
                        1);

  clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                               (ClutterActor *)priv->primary_text,
                               "x-expand",
                               TRUE,
                               "y-expand",
                               FALSE,
                               NULL);

  nbtk_table_add_actor (NBTK_TABLE (priv->details_overlay),
                        (ClutterActor *)priv->secondary_text,
                        1,
                        1);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                               (ClutterActor *)priv->secondary_text,
                               "x-expand",
                               TRUE,
                               "y-expand",
                               FALSE,
                               NULL);

  /*
   * Explicitly set the width to 100 to avoid overflowing text. Slightly
   * hackyish but works around a strange bug in NbtkTable where if the text
   * is too long it will cause negative positioning of the icon.
   */
  clutter_actor_set_width ((ClutterActor *)priv->primary_text, 100);
  clutter_actor_set_width ((ClutterActor *)priv->secondary_text, 100);

  nbtk_table_add_actor (NBTK_TABLE (priv->details_overlay),
                        priv->icon,
                        0,
                        0);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->details_overlay),
                               priv->icon,
                               "row-span",
                               2,
                               "y-expand",
                               FALSE,
                               "x-expand",
                               FALSE,
                               "y-fill",
                               FALSE,
                               "x-fill",
                               FALSE,
                               NULL);

  g_signal_connect (self,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    self);
  g_signal_connect (self,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    self);

  nbtk_table_set_col_spacing (NBTK_TABLE (priv->details_overlay), 4);


  clutter_actor_set_reactive ((ClutterActor *) self, TRUE);
}

void
penge_people_tile_activate (PengePeopleTile *tile,
                            MojitoItem      *item)
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
penge_people_tile_set_item (PengePeopleTile *tile,
                            MojitoItem      *item)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (tile);
  ClutterActor *body, *tmp_text;
  NbtkWidget *label;
  const gchar *content, *thumbnail;
  gchar *date;
  GError *error = NULL;

  if (priv->item == item)
    return;

  if (priv->item)
    mojito_item_unref (priv->item);

  if (item)
    priv->item = mojito_item_ref (item);
  else
    priv->item = NULL;

  if (!priv->item)
    return;

  if (mojito_item_has_key (item, "thumbnail"))
  {
    thumbnail = mojito_item_get_value (item, "thumbnail");
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
  } else if (mojito_item_has_key (item, "content")) {
    content = mojito_item_get_value (item, "content");
    body = (ClutterActor *)nbtk_bin_new ();
    nbtk_widget_set_style_class_name ((NbtkWidget *)body,
                                      "PengePeopleTileContentBackground");
    label = nbtk_label_new (content);
    nbtk_widget_set_style_class_name (label, "PengePeopleTileContentLabel");
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
  } else {
    if (g_str_equal (item->service,"lastfm"))
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

  if (mojito_item_has_key (item, "title"))
  {
    if (mojito_item_has_key (item, "author"))
    {
      g_object_set (tile,
                    "primary-text", mojito_item_get_value (item, "title"),
                    "secondary-text", mojito_item_get_value (item, "author"),
                    NULL);
    } else {
      date = mpl_utils_format_time (&(item->date));
      g_object_set (tile,
                    "primary-text", mojito_item_get_value (item, "title"),
                    "secondary-text", date,
                    NULL);
      g_free (date);
    }
  } else if (mojito_item_has_key (item, "author")) {
      date = mpl_utils_format_time (&(item->date));
      g_object_set (tile,
                    "primary-text", mojito_item_get_value (item, "author"),
                    "secondary-text", date,
                    NULL);
      g_free (date);
  } else {
    g_assert_not_reached ();
  }

  g_object_set (tile,
                "icon-path", mojito_item_get_value (item, "authoricon"),
                NULL);
}

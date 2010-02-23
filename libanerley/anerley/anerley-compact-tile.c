/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2010, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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
 *
 */

#include "anerley-compact-tile.h"
#include <anerley/anerley-item.h>

G_DEFINE_TYPE (AnerleyCompactTile, anerley_compact_tile, MX_TYPE_WIDGET)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_COMPACT_TILE, AnerleyCompactTilePrivate))

typedef struct _AnerleyCompactTilePrivate AnerleyCompactTilePrivate;

struct _AnerleyCompactTilePrivate {
  AnerleyItem *item;
  ClutterActor *contact_name_label;
  ClutterActor *presence_icon;
};

enum
{
  PROP_0,
  PROP_ITEM
};

static void anerley_compact_tile_set_item (AnerleyCompactTile *tile,
                                           AnerleyItem        *item);

static void
anerley_compact_tile_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
  case PROP_ITEM:
    g_value_set_object (value, priv->item);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_compact_tile_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  AnerleyCompactTile *tile = ANERLEY_COMPACT_TILE (object);

  switch (property_id) {
    case PROP_ITEM:
      anerley_compact_tile_set_item (tile, g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_compact_tile_dispose (GObject *object)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (object);

  if (priv->item)
  {
    g_object_unref (priv->item);
    priv->item = NULL;
  }

  G_OBJECT_CLASS (anerley_compact_tile_parent_class)->dispose (object);
}

static void
anerley_compact_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_compact_tile_parent_class)->finalize (object);
}

#define SPACING 8

static void
anerley_compact_tile_allocate (ClutterActor           *actor,
                               const ClutterActorBox  *box,
                               ClutterAllocationFlags  flags)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (actor);
  ClutterActorBox icon_box, label_box;
  gfloat width, height;
  gfloat icon_nat_w, icon_min_w, label_nat_w, label_min_w;
  gfloat icon_nat_h, icon_min_h, label_nat_h, label_min_h;
  MxPadding padding;

  CLUTTER_ACTOR_CLASS (anerley_compact_tile_parent_class)->allocate (actor,
                                                                     box,
                                                                     flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  clutter_actor_get_preferred_size (priv->presence_icon,
                                    &icon_min_w,
                                    &icon_min_h,
                                    &icon_nat_w,
                                    &icon_nat_h);

  icon_box.x1 = padding.left;
  icon_box.x2 = icon_box.x1 + icon_nat_w;
  icon_box.y1 = (gint)((height - icon_nat_h) / 2);
  icon_box.y2 =  icon_box.y1 + icon_nat_h;
  clutter_actor_allocate (priv->presence_icon, &icon_box, flags);

  clutter_actor_get_preferred_size (priv->contact_name_label,
                                    &label_min_w,
                                    &label_min_h,
                                    &label_nat_w,
                                    &label_nat_h);

  label_box.x1 = icon_box.x2 + SPACING;
  label_box.x2 = width - padding.right;
  label_box.y1 = (gint)((height - label_nat_h) / 2);
  label_box.y2 = label_box.y1 + label_nat_h;
  clutter_actor_allocate (priv->contact_name_label, &label_box, flags);
}

static void
anerley_compact_tile_get_preferred_width (ClutterActor *actor,
                                          gfloat        for_height,
                                          gfloat       *min_width,
                                          gfloat       *nat_width)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (actor);
  MxPadding padding;
  gfloat icon_nat_w, icon_min_w, label_nat_w, label_min_w;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  clutter_actor_get_preferred_width (priv->presence_icon,
                                     for_height,
                                     &icon_min_w,
                                     &icon_nat_w);
  clutter_actor_get_preferred_width (priv->contact_name_label,
                                     for_height,
                                     &label_min_w,
                                     &label_nat_w);

  if (min_width)
  {
    *min_width = padding.left + icon_min_w + SPACING + label_min_w + padding.right;
  }

  if (nat_width)
  {
    *nat_width = padding.left + icon_nat_w + SPACING + label_nat_w + padding.right;
  }
}

static void
anerley_compact_tile_get_preferred_height (ClutterActor *actor,
                                           gfloat        for_width,
                                           gfloat       *min_height,
                                           gfloat       *nat_height)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (actor);
  MxPadding padding;
  gfloat icon_nat_h, icon_min_h, label_nat_h, label_min_h;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  clutter_actor_get_preferred_height (priv->presence_icon,
                                      for_width,
                                      &icon_min_h,
                                      &icon_nat_h);
  clutter_actor_get_preferred_height (priv->contact_name_label,
                                      for_width,
                                      &label_min_h,
                                      &label_nat_h);

  if (min_height)
  {
    *min_height = padding.top + MAX (label_min_h, icon_min_h) + padding.bottom;
    g_debug (G_STRLOC ": min_height = %f", *min_height);
  }

  if (nat_height)
  {
    *nat_height = padding.top + MAX (label_nat_h, icon_nat_h) + padding.bottom;
    g_debug (G_STRLOC ": nat_height = %f", *nat_height);
  }
}

static void
anerley_compact_tile_map (ClutterActor *actor)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (anerley_compact_tile_parent_class)->map (actor);

  clutter_actor_map (priv->presence_icon);
  clutter_actor_map (priv->contact_name_label);
}

static void
anerley_compact_tile_unmap (ClutterActor *actor)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (anerley_compact_tile_parent_class)->unmap (actor);

  clutter_actor_unmap (priv->presence_icon);
  clutter_actor_unmap (priv->contact_name_label);
}

static void
anerley_compact_tile_paint (ClutterActor *actor)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (anerley_compact_tile_parent_class)->paint (actor);

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->presence_icon))
  {
    clutter_actor_paint (priv->presence_icon);
  }

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->contact_name_label))
    clutter_actor_paint (priv->contact_name_label);
}

static void
anerley_compact_tile_pick (ClutterActor       *actor,
                           const ClutterColor *color)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (anerley_compact_tile_parent_class)->pick (actor, color);

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->presence_icon))
    clutter_actor_paint (priv->presence_icon);

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->contact_name_label))
    clutter_actor_paint (priv->contact_name_label);
}

static void
anerley_compact_tile_class_init (AnerleyCompactTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyCompactTilePrivate));

  object_class->get_property = anerley_compact_tile_get_property;
  object_class->set_property = anerley_compact_tile_set_property;
  object_class->dispose = anerley_compact_tile_dispose;
  object_class->finalize = anerley_compact_tile_finalize;

  actor_class->map = anerley_compact_tile_map;
  actor_class->unmap = anerley_compact_tile_unmap;
  actor_class->paint = anerley_compact_tile_paint;
  actor_class->pick = anerley_compact_tile_pick;
  actor_class->get_preferred_width = anerley_compact_tile_get_preferred_width;
  actor_class->get_preferred_height = anerley_compact_tile_get_preferred_height;
  actor_class->allocate = anerley_compact_tile_allocate;

  pspec = g_param_spec_object ("item",
                               "Item",
                               "Item to present",
                               ANERLEY_TYPE_ITEM,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);
}

static void
anerley_compact_tile_init (AnerleyCompactTile *self)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (self);

  priv->contact_name_label = mx_label_new ("");
  clutter_actor_set_parent (priv->contact_name_label,
                            CLUTTER_ACTOR (self));

  priv->presence_icon = clutter_texture_new ();
  clutter_actor_set_size (priv->presence_icon, 16, 16);
  clutter_actor_set_parent (priv->presence_icon, CLUTTER_ACTOR (self));

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);
}

static void
anerley_compact_tile_update_presence_icon (AnerleyCompactTile *tile,
                                           const gchar        *icon_name)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (tile);
  CoglHandle tex_h;

  tex_h = mx_icon_theme_lookup (mx_icon_theme_get_default (),
                                icon_name,
                                16);
  clutter_texture_set_cogl_texture (CLUTTER_TEXTURE (priv->presence_icon),
                                    tex_h);
}

static void
_item_presence_changed_cb (AnerleyItem *item,
                           gpointer     userdata)
{
  AnerleyCompactTile *tile = (AnerleyCompactTile *)userdata;
  const gchar *presence_status;
  const gchar *presence_icon_name;
  const gchar *tmp;

  presence_status = anerley_item_get_presence_status (item);
  tmp = g_intern_string ((gchar *)presence_status);

  if (tmp == g_intern_static_string ("available"))
    presence_icon_name = "user-available";
  else if (tmp == g_intern_static_string ("dnd"))
    presence_icon_name = "user-busy";
  else if (tmp == g_intern_static_string ("away"))
    presence_icon_name = "user-away";
  else if (tmp == g_intern_static_string ("xa"))
    presence_icon_name = "user-away";
  else
    presence_icon_name = "user-offline";

  anerley_compact_tile_update_presence_icon (tile,
                                             presence_icon_name);
}

static void
_item_display_name_changed_cb (AnerleyItem *item,
                               gpointer     userdata)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (userdata);

  mx_label_set_text (MX_LABEL (priv->contact_name_label),
                     anerley_item_get_display_name (priv->item));
}

static void
anerley_compact_tile_set_item (AnerleyCompactTile *tile,
                               AnerleyItem        *item)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (tile);

  if (priv->item == item)
    return;

  if (priv->item)
  {
    g_signal_handlers_disconnect_by_func (priv->item,
                                          _item_display_name_changed_cb,
                                          tile);
    g_signal_handlers_disconnect_by_func (priv->item,
                                          _item_presence_changed_cb,
                                          tile);
    g_object_unref (priv->item);
    priv->item = NULL;
  }

  if (item)
  {
    priv->item = g_object_ref (item);
    g_signal_connect (priv->item,
                      "display-name-changed",
                      (GCallback)_item_display_name_changed_cb,
                      tile);
    g_signal_connect (priv->item,
                      "presence-changed",
                      (GCallback)_item_presence_changed_cb,
                      tile);

    anerley_item_emit_display_name_changed (item);
    anerley_item_emit_presence_changed (item);
  }
}

AnerleyItem *
anerley_compact_tile_get_item (AnerleyCompactTile *tile)
{
  AnerleyCompactTilePrivate *priv = GET_PRIVATE (tile);

  return priv->item;
}

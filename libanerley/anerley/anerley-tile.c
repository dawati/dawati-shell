/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2009, Intel Corporation.
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


#include "penge-magic-texture.h"

#include "anerley-tile.h"
#include "anerley-item.h"
#include "anerley-tile-view.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (AnerleyTile, anerley_tile, MX_TYPE_TABLE)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_TILE, AnerleyTilePrivate))

#define GET_PRIVATE(o) \
    ((AnerleyTile *)o)->priv


struct _AnerleyTilePrivate {
  AnerleyItem *item;

  ClutterActor *avatar;
  ClutterActor *avatar_frame;
  ClutterActor *primary_label;
  ClutterActor *secondary_label;
  ClutterActor *presence_label;
  ClutterActor *presence_icon;
};

enum
{
  PROP_0 = 0,
  PROP_ITEM
};

#define DEFAULT_AVATAR_IMAGE PKG_DATA_DIR "/" "default-avatar-image.png"

static void
_item_display_name_changed_cb (AnerleyItem *item,
                               gpointer     userdata)
{
  AnerleyTilePrivate *priv = GET_PRIVATE (userdata);

  mx_label_set_text (MX_LABEL (priv->primary_label),
                     anerley_item_get_display_name (priv->item));
}

static CoglHandle 
_get_default_avatar_texture (void)
{
  MxTextureCache *cache;
  ClutterTexture *tmp_tex;
  static CoglHandle handle = NULL;

  if (handle)
    return handle;

  cache = mx_texture_cache_get_default ();
  tmp_tex = mx_texture_cache_get_texture (cache,
                                          DEFAULT_AVATAR_IMAGE);
  handle = clutter_texture_get_cogl_texture (tmp_tex);
  cogl_handle_ref (handle);
  g_object_unref (tmp_tex);

  return handle;
}

static void
_item_avatar_path_changed_cb (AnerleyItem *item,
                              gpointer     userdata)
{
  AnerleyTilePrivate *priv = GET_PRIVATE (userdata);
  GError *error = NULL;
  const gchar *avatar_path = NULL;

  avatar_path = anerley_item_get_avatar_path (priv->item);

  if (!avatar_path)
  {
    clutter_texture_set_cogl_texture ((ClutterTexture *)priv->avatar,
                                      _get_default_avatar_texture());
  } else{
    if (!clutter_texture_set_from_file ((ClutterTexture *)priv->avatar,
                                        avatar_path,
                                        &error))
    {
      g_warning (G_STRLOC ": Error opening avatar image: %s",
                 error->message);
      g_clear_error (&error);
    }
  }
}

static void
anerley_tile_update_presence_icon (AnerleyTile *tile,
                                   const gchar *path)
{
  AnerleyTilePrivate *priv = GET_PRIVATE (tile);
  MxTextureCache *cache;

  if (priv->presence_icon)
  {
    clutter_container_remove_actor (CLUTTER_CONTAINER (tile),
                                    priv->presence_icon);
  }

  cache = mx_texture_cache_get_default ();

  priv->presence_icon = (ClutterActor *)mx_texture_cache_get_texture (cache,
                                                                      path);
  clutter_actor_set_size (priv->presence_icon, 16, 16);
  mx_table_add_actor_with_properties (MX_TABLE (tile),
                                      priv->presence_icon,
                                      2,
                                      1,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "y-align", 0.0,
                                      NULL);

  clutter_actor_show (priv->presence_icon);
}

static void
_item_presence_changed_cb (AnerleyItem *item,
                           gpointer     userdata)
{
  AnerleyTile *tile = (AnerleyTile *)userdata;
  AnerleyTilePrivate *priv = GET_PRIVATE (userdata);
  const gchar *presence_message;
  const gchar *presence_status;
  const gchar *tmp;
  const gchar *presence_icon_name;
  gchar *presence_icon_path;

  presence_message = anerley_item_get_presence_message (item);
  presence_status = anerley_item_get_presence_status (item);
  tmp = g_intern_string ((gchar *)presence_status);

  if (presence_status == NULL)
  {
    clutter_actor_hide (priv->presence_label);
    if (priv->presence_icon)
      clutter_actor_hide (priv->presence_icon);

    return;
  }

  if (presence_message && !g_str_equal (presence_message, ""))
  {
    mx_label_set_text ((MxLabel *)priv->presence_label,
                       presence_message);
  } else {
    if (tmp == g_intern_static_string ("available"))
      mx_label_set_text ((MxLabel *)priv->presence_label,
                         _("Available"));
    else if (tmp == g_intern_static_string ("dnd"))
      mx_label_set_text ((MxLabel *)priv->presence_label,
                         _("Busy"));
    else if (tmp == g_intern_static_string ("away"))
      mx_label_set_text ((MxLabel *)priv->presence_label,
                         _("Away"));
    else if (tmp == g_intern_static_string ("xa"))
      mx_label_set_text ((MxLabel *)priv->presence_label,
                         _("Away"));
    else
      mx_label_set_text ((MxLabel *)priv->presence_label,
                         _("Offline"));
  }

  clutter_actor_show (priv->presence_label);

  if (tmp == g_intern_static_string ("available"))
    presence_icon_name = "presence-available.png";
  else if (tmp == g_intern_static_string ("dnd"))
    presence_icon_name = "presence-busy.png";
  else if (tmp == g_intern_static_string ("away"))
    presence_icon_name = "presence-away.png";
  else if (tmp == g_intern_static_string ("xa"))
    presence_icon_name = "presence-away.png";
  else
    presence_icon_name = "presence-offline.png";

  presence_icon_path = g_build_filename (PKG_DATA_DIR,
                                         presence_icon_name,
                                         NULL);

  anerley_tile_update_presence_icon (tile, presence_icon_path);

  g_free (presence_icon_path);
}

static void
anerley_tile_update_item (AnerleyTile *tile,
                          AnerleyItem *item)
{
  AnerleyTilePrivate *priv = GET_PRIVATE (tile);

  if (priv->item == item)
    return;

  if (priv->item)
  {
    g_signal_handlers_disconnect_by_func (priv->item,
                                          _item_display_name_changed_cb,
                                          tile);
    g_signal_handlers_disconnect_by_func (priv->item,
                                          _item_avatar_path_changed_cb,
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
                      "avatar-path-changed",
                      (GCallback)_item_avatar_path_changed_cb,
                      tile);
    g_signal_connect (priv->item,
                      "presence-changed",
                      (GCallback)_item_presence_changed_cb,
                      tile);

    if (CLUTTER_ACTOR_IS_MAPPED (tile))
    {
      anerley_item_emit_display_name_changed (item);
      anerley_item_emit_avatar_path_changed (item);
      anerley_item_emit_presence_changed (item);
    }
  }
}

static void
anerley_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_ITEM:
      g_value_set_object (value, priv->item);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PROP_ITEM:
      anerley_tile_update_item ((AnerleyTile *)object,
                                g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tile_dispose (GObject *object)
{
  AnerleyTile *tile = (AnerleyTile *)object;
  AnerleyTilePrivate *priv = GET_PRIVATE (object);

  if (priv->item)
  {
    anerley_tile_update_item (tile, NULL);
  }

  G_OBJECT_CLASS (anerley_tile_parent_class)->dispose (object);
}

static void
anerley_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_tile_parent_class)->finalize (object);
}

static void
anerley_tile_map (ClutterActor *actor)
{
  AnerleyTilePrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (anerley_tile_parent_class)->map (actor);

  if (priv->item)
  {
    anerley_item_emit_display_name_changed (priv->item);
    anerley_item_emit_avatar_path_changed (priv->item);
    anerley_item_emit_presence_changed (priv->item);
  }
}

static void
anerley_tile_allocate (ClutterActor           *actor,
                       const ClutterActorBox  *box,
                       ClutterAllocationFlags  flags)
{
  ClutterActorClass  *actor_class;


  if (CLUTTER_ACTOR_IS_MAPPED (actor))
  {
    actor_class = CLUTTER_ACTOR_CLASS (anerley_tile_parent_class);
  } else {
    actor_class = g_type_class_peek (CLUTTER_TYPE_ACTOR);
  }

  actor_class->allocate (actor, box, flags);
}

static void
anerley_tile_get_preferred_width (ClutterActor *actor,
                                  gfloat        for_height,
                                  gfloat       *min_width,
                                  gfloat       *pref_width)
{
  if (min_width)
    *min_width = 180;
  if (pref_width)
    *pref_width = 180;
}

static void
anerley_tile_get_preferred_height (ClutterActor *actor,
                                   gfloat        for_width,
                                   gfloat       *min_height,
                                   gfloat       *pref_height)
{
  if (min_height)
    *min_height = 90;
  if (pref_height)
    *pref_height = 90;
}


static void
anerley_tile_class_init (AnerleyTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyTilePrivate));

  object_class->get_property = anerley_tile_get_property;
  object_class->set_property = anerley_tile_set_property;
  object_class->dispose = anerley_tile_dispose;
  object_class->finalize = anerley_tile_finalize;

  actor_class->map = anerley_tile_map;
  actor_class->allocate = anerley_tile_allocate;
  actor_class->get_preferred_width = anerley_tile_get_preferred_width;
  actor_class->get_preferred_height = anerley_tile_get_preferred_height;

  pspec = g_param_spec_object ("item",
                               "Item",
                               "The item to show",
                               ANERLEY_TYPE_ITEM,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);
}

static gboolean
_button_press_event_cb (ClutterActor *actor,
                        ClutterEvent *event,
                        gpointer      userdata)
{
  AnerleyTilePrivate *priv = GET_PRIVATE (actor);
  ClutterActor *parent;

  if (((ClutterButtonEvent *)event)->click_count == 2)
  {
    parent = clutter_actor_get_parent (actor);

    if (ANERLEY_IS_TILE_VIEW (parent))
    {
      g_signal_emit_by_name (parent,
                             "item-activated",
                             priv->item);
    }
  }

  return TRUE;
}

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer       userdata)
{
  mx_stylable_set_style_pseudo_class (MX_STYLABLE (actor),
                                      "hover");
  return TRUE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  mx_stylable_set_style_pseudo_class (MX_STYLABLE (actor),
                                      NULL);
  return TRUE;
}

static void
anerley_tile_init (AnerleyTile *self)
{
  AnerleyTilePrivate *priv = GET_PRIVATE_REAL (self);
  ClutterActor *tmp_text;

  self->priv = priv;

  priv->avatar_frame = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->avatar_frame),
                               "AnerleyTileAvatar");

  priv->avatar = g_object_new (PENGE_TYPE_MAGIC_TEXTURE, NULL);
  /* TODO: Prefill with unknown icon */
  mx_bin_set_child (MX_BIN (priv->avatar_frame), priv->avatar);

  /* add avatar */
  mx_table_add_actor (MX_TABLE (self),
                      priv->avatar_frame, 0,
                      0);
  mx_table_child_set_x_fill (MX_TABLE (self),
                             priv->avatar_frame, FALSE);
  mx_table_child_set_y_fill (MX_TABLE (self),
                             priv->avatar_frame, FALSE);
  mx_table_child_set_x_expand (MX_TABLE (self),
                               priv->avatar_frame, FALSE);
  mx_table_child_set_y_expand (MX_TABLE (self),
                               priv->avatar_frame, FALSE);
  mx_table_child_set_row_span (MX_TABLE (self),
                               priv->avatar_frame, 3);

  priv->primary_label = mx_label_new ("");
  mx_stylable_set_style_class (MX_STYLABLE (priv->primary_label),
                                    "AnerleyTilePrimaryLabel");

  mx_table_add_actor (MX_TABLE (self),
                      priv->primary_label,
                      0,
                      1);
  mx_table_child_set_y_fill (MX_TABLE (self),
                             priv->primary_label, FALSE);
  mx_table_child_set_x_expand (MX_TABLE (self),
                               priv->primary_label, FALSE);
  mx_table_child_set_col_span (MX_TABLE (self),
                               priv->primary_label, 2);
  mx_table_child_set_y_align (MX_TABLE (self),
                              priv->primary_label, 1.0);
  mx_table_child_set_x_align (MX_TABLE (self),
                              priv->primary_label, 0.0);
  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->primary_label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text), PANGO_WRAP_WORD);

  priv->presence_label = mx_label_new ("");
  mx_stylable_set_style_class (MX_STYLABLE (priv->presence_label),
                               "AnerleyTilePresenceLabel");

  mx_table_add_actor (MX_TABLE (self),
                      (ClutterActor *)priv->presence_label, 2, 2);
  mx_table_child_set_y_fill (MX_TABLE (self),
                             priv->presence_label, FALSE);
  mx_table_child_set_x_fill (MX_TABLE (self),
                             priv->presence_label, FALSE);
  mx_table_child_set_y_align (MX_TABLE (self),
                              priv->presence_label, 0.0);
  mx_table_child_set_x_align (MX_TABLE (self),
                              priv->presence_label, 0.0);

  mx_table_set_col_spacing ((MxTable *)self,
                            4);
  mx_table_set_row_spacing ((MxTable *)self,
                            4);

  g_signal_connect (self,
                    "button-press-event",
                    (GCallback)_button_press_event_cb,
                    NULL);

  g_signal_connect (self,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    NULL);
  g_signal_connect (self,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    NULL);

  clutter_actor_set_reactive ((ClutterActor *)self, TRUE);
}

ClutterActor *
anerley_tile_new (AnerleyItem *item)
{
  return g_object_new (ANERLEY_TYPE_TILE,
                       "item",
                       item,
                       NULL);
}

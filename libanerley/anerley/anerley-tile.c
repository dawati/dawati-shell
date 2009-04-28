#include "anerley-tile.h"
#include "anerley-item.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (AnerleyTile, anerley_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_TILE, AnerleyTilePrivate))

typedef struct _AnerleyTilePrivate AnerleyTilePrivate;

struct _AnerleyTilePrivate {
  AnerleyItem *item;

  ClutterActor *avatar;
  NbtkWidget *avatar_bin;
  NbtkWidget *primary_label;
  NbtkWidget *secondary_label;
  ClutterActor *presence_icon;
  NbtkWidget *presence_label;
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
  gchar **splits;

  if (!anerley_item_get_display_name (item))
    return;

  splits = g_strsplit (anerley_item_get_display_name (item),
                       " ",
                       2);

  if (splits && splits[0])
  {
    nbtk_label_set_text ((NbtkLabel *)priv->primary_label,
                         (gchar *)splits[0]);
    g_free (splits[0]);
  }

  if (splits && splits[1])
  {
    nbtk_label_set_text ((NbtkLabel *)priv->secondary_label,
                         (gchar *)splits[1]);
    g_free (splits[1]);
  }

  g_free (splits);
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
    avatar_path = DEFAULT_AVATAR_IMAGE;
  }

  if (!clutter_texture_set_from_file ((ClutterTexture *)priv->avatar,
                                      avatar_path,
                                      &error))
  {
    g_warning (G_STRLOC ": Error opening avatar image: %s",
               error->message);
    g_clear_error (&error);
  }
}

static void
_item_presence_changed_cb (AnerleyItem *item,
                           gpointer     userdata)
{
  AnerleyTilePrivate *priv = GET_PRIVATE (userdata);
  const gchar *presence_message;
  const gchar *presence_status;
  const gchar *tmp;
  const gchar *presence_icon_name;
  gchar *presence_icon_path;
  GError *error = NULL;

  presence_message = anerley_item_get_presence_message (item);
  presence_status = anerley_item_get_presence_status (item);
  tmp = g_intern_string ((gchar *)presence_status);

  if (presence_message && !g_str_equal (presence_message, ""))
  {
    nbtk_label_set_text ((NbtkLabel *)priv->presence_label,
                         presence_message);
  } else {
    if (tmp == g_intern_static_string ("available"))
      nbtk_label_set_text ((NbtkLabel *)priv->presence_label,
                           _("Available"));
    else if (tmp == g_intern_static_string ("dnd"))
      nbtk_label_set_text ((NbtkLabel *)priv->presence_label,
                           _("Busy"));
    else if (tmp == g_intern_static_string ("away"))
      nbtk_label_set_text ((NbtkLabel *)priv->presence_label,
                           _("Away"));
    else if (tmp == g_intern_static_string ("xa"))
      nbtk_label_set_text ((NbtkLabel *)priv->presence_label,
                           _("Away"));
    else
      nbtk_label_set_text ((NbtkLabel *)priv->presence_label,
                           _("Offline"));
  }

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

  if (!clutter_texture_set_from_file ((ClutterTexture *)priv->presence_icon,
                                      presence_icon_path,
                                      &error))
  {
    g_warning (G_STRLOC ": Error opening presence icon: %s",
               error->message);
    g_clear_error (&error);
  }

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

    anerley_item_emit_display_name_changed (item);
    anerley_item_emit_avatar_path_changed (item);
    anerley_item_emit_presence_changed (item);
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
anerley_tile_class_init (AnerleyTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyTilePrivate));

  object_class->get_property = anerley_tile_get_property;
  object_class->set_property = anerley_tile_set_property;
  object_class->dispose = anerley_tile_dispose;
  object_class->finalize = anerley_tile_finalize;

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

  anerley_item_activate (priv->item);

  return TRUE;
}

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer       userdata)
{
  nbtk_widget_set_style_pseudo_class ((NbtkWidget *)actor,
                                      "hover");
  return TRUE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  nbtk_widget_set_style_pseudo_class ((NbtkWidget *)actor,
                                      NULL);
  return TRUE;
}

static void
anerley_tile_init (AnerleyTile *self)
{
  AnerleyTilePrivate *priv = GET_PRIVATE (self);

  priv->avatar_bin = nbtk_bin_new ();
  nbtk_widget_set_style_class_name (priv->avatar_bin, "AnerleyTileAvatar");

  priv->avatar = clutter_texture_new ();
  clutter_actor_set_size (priv->avatar, 48, 48);
  /* TODO: Prefill with unknown icon */
  nbtk_bin_set_child (NBTK_BIN (priv->avatar_bin), priv->avatar);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)priv->avatar_bin,
                                        0,
                                        0,
                                        "x-fill",
                                        FALSE,
                                        "y-fill",
                                        FALSE,
                                        "x-expand",
                                        FALSE,
                                        "y-expand",
                                        FALSE,
                                        "row-span",
                                        3,
                                        NULL);

  priv->primary_label = nbtk_label_new ("");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)priv->primary_label,
                                        0,
                                        1,
                                        "x-fill",
                                        TRUE,
                                        "y-fill",
                                        FALSE,
                                        "y-expand",
                                        TRUE,
                                        "x-expand",
                                        FALSE,
                                        "x-align",
                                        0.0,
                                        "y-align",
                                        1.0,
                                        "col-span",
                                        2,
                                        NULL);
  nbtk_widget_set_style_class_name (priv->primary_label,
                                    "AnerleyTilePrimaryLabel");

  priv->secondary_label = nbtk_label_new ("");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)priv->secondary_label,
                                        1,
                                        1,
                                        "x-fill",
                                        TRUE,
                                        "y-fill",
                                        FALSE,
                                        "y-expand",
                                        FALSE,
                                        "x-expand",
                                        FALSE,
                                        "x-align",
                                        0.0,
                                        "y-align",
                                        0.0,
                                        "col-span",
                                        2,
                                        NULL);
  nbtk_widget_set_style_class_name (priv->secondary_label,
                                    "AnerleyTileSecondaryLabel");

  priv->presence_icon = clutter_texture_new ();
  clutter_actor_set_size (priv->presence_icon, 16, 16);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)priv->presence_icon,
                                        2,
                                        1,
                                        "x-fill",
                                        FALSE,
                                        "y-fill",
                                        FALSE,
                                        "x-expand",
                                        FALSE,
                                        "y-expand",
                                        TRUE,
                                        "y-align",
                                        0.0,
                                        NULL);
  priv->presence_label = nbtk_label_new ("");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)priv->presence_label,
                                        2,
                                        2,
                                        "x-fill",
                                        FALSE,
                                        "y-fill",
                                        FALSE,
                                        "y-expand",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "x-align",
                                        0.0,
                                        "y-align",
                                        0.0,
                                        NULL);
  nbtk_widget_set_style_class_name (priv->presence_label,
                                    "AnerleyTilePresenceLabel");
  nbtk_table_set_col_spacing ((NbtkTable *)self,
                              4);
  nbtk_table_set_row_spacing ((NbtkTable *)self,
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
}

NbtkWidget *
anerley_tile_new (AnerleyItem *item)
{
  return g_object_new (ANERLEY_TYPE_TILE,
                       "item",
                       item,
                       NULL);
}

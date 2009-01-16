#include <mojito-client/mojito-item.h>
#include <nbtk/nbtk.h>
#include <gio/gio.h>

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


static void
_button_press_event (ClutterActor *actor,
                     ClutterEvent *event,
                     gpointer      userdata)
{
  PengeTwitterTilePrivate *priv = GET_PRIVATE (userdata);
  const gchar *url;
  GError *error = NULL;

  url = g_hash_table_lookup (priv->item->props,
                             "url");

  if (!g_app_info_launch_default_for_uri (url,
                                     NULL,
                                     &error))
  {
    g_warning (G_STRLOC ": Error launching uri (%s): %s",
               url,
               error->message);
    g_clear_error (&error);
  } else {
    penge_utils_signal_activated (actor);
  }
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
  ClutterActor *label;

  g_return_if_fail (priv->item != NULL);

  content = g_hash_table_lookup (priv->item->props,
                                 "content");
  author = g_hash_table_lookup (priv->item->props,
                                "author");
  authoricon_path = g_hash_table_lookup (priv->item->props,
                                         "authoricon");

  date = penge_utils_format_time (&(priv->item->date));

  g_object_set (tile,
                "primary-text",
                author,
                "secondary-text",
                date,
                "icon-path",
                authoricon_path,
                NULL);
  g_free (date);

  body = nbtk_label_new (content);
  nbtk_widget_set_style_class_name (body, "PengeTwitterTileLabel");
  label = nbtk_label_get_clutter_label (NBTK_LABEL (body));
  clutter_label_set_line_wrap (CLUTTER_LABEL (label), TRUE);
  clutter_label_set_ellipsize (CLUTTER_LABEL (label),
                               PANGO_ELLIPSIZE_NONE);
  clutter_label_set_alignment (CLUTTER_LABEL (label),
                               PANGO_ALIGN_LEFT);
  nbtk_widget_set_alignment (NBTK_WIDGET (body),
                             0,
                             0);
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


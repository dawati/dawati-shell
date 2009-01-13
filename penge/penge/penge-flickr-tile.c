#include <gio/gio.h>
#include <mojito-client/mojito-item.h>

#include "penge-utils.h"
#include "penge-flickr-tile.h"

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

static void
_button_press_event (ClutterActor *actor,
                     ClutterEvent *event,
                     gpointer      userdata)
{
  PengeFlickrTilePrivate *priv = GET_PRIVATE (userdata);
  const gchar *url;
  GError *error = NULL;

  url = g_hash_table_lookup (priv->item->props,
                             "url");

  if (!g_app_info_launch_default_for_uri (url,
                                     NULL,
                                     &error))
  {
    g_warning (G_STRLOC ": Error launching uri: %s",
               error->message);
    g_clear_error (&error);
  } else {
    penge_utils_signal_activated (actor);
  }
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
  date = penge_utils_format_time (&(priv->item->date));
  g_object_set (tile,
                "primary-text",
                title,
                "secondary-text",
                author,
                "icon-path",
                buddyicon_path,
                NULL);
  g_free (date);

  body = clutter_texture_new_from_file (thumbnail_path, &error);

  if (body)
  {
    g_object_set (tile,
                  "body",
                  body,
                  NULL);
    clutter_container_child_set (CLUTTER_CONTAINER (tile),
                                 body,
                                 "keep-aspect-ratio",
                                 TRUE,
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



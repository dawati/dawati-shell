#include <mojito-client/mojito-client.h>

#include "penge-utils.h"

#include "penge-flickr-tile.h"
#include "penge-twitter-tile.h"

#include "penge-people-pane.h"

G_DEFINE_TYPE (PengePeoplePane, penge_people_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_PANE, PengePeoplePanePrivate))

typedef struct _PengePeoplePanePrivate PengePeoplePanePrivate;

struct _PengePeoplePanePrivate {
  MojitoClient *client;
  MojitoClientView *view;
  GHashTable *uuid_to_actor;
};

#define NUMBER_COLS 2
#define MAX_ITEMS 6

static void
penge_people_pane_dispose (GObject *object)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (object);

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }

  if (priv->view)
  {
    g_object_unref (priv->view);
    priv->view = NULL;
  }

  G_OBJECT_CLASS (penge_people_pane_parent_class)->dispose (object);
}

static void
penge_people_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_pane_parent_class)->finalize (object);
}

static ClutterActor *
penge_people_pane_fabricate_actor (PengePeoplePane *pane,
                                   MojitoItem      *item)
{
  ClutterActor *actor;
  ClutterColor blue = {0, 0, 255, 127};

  if (g_str_equal (item->source, "flickr"))
  {
    actor = g_object_new (PENGE_TYPE_FLICKR_TILE,
                          "item",
                          item,
                          NULL);

  } else if (g_str_equal (item->source, "twitter")) {
    actor = g_object_new (PENGE_TYPE_TWITTER_TILE,
                          "item",
                          item,
                          NULL);
  } else {
    actor = clutter_rectangle_new ();
    clutter_rectangle_set_color (CLUTTER_RECTANGLE (actor),
                               &blue);
  }

  clutter_actor_set_size (actor, 150, 150);
  
  return actor;
}

/*
 * This function iterates over the list of items that are apparently in the
 * view and packs them into the table.
 *
 * If an item that is not seen before comes in then a new actor of the
 * appropriate type is fabricated in the factory.
 */
static void
penge_people_pane_update (PengePeoplePane *pane)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (pane);

  GList *items, *l;
  MojitoItem *item;
  gint count = 0;
  ClutterActor *actor;

  items = mojito_client_view_get_sorted_items (priv->view);

  for (l = items; l; l = l->next)
  {
    item = (MojitoItem *)l->data;
    actor = g_hash_table_lookup (priv->uuid_to_actor, item->uuid);

    if (!actor)
    {
      actor = penge_people_pane_fabricate_actor (pane, item);
      nbtk_table_add_actor (NBTK_TABLE (pane),
                            actor,
                            count / NUMBER_COLS,
                            count % NUMBER_COLS);
      g_hash_table_insert (priv->uuid_to_actor,
                           g_strdup (item->uuid),
                           g_object_ref (actor));
      g_debug (G_STRLOC ": Putting new item (%s) in (%d, %d)",
               item->uuid,
               count / NUMBER_COLS,
               count / NUMBER_COLS);
    } else {
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   actor,
                                   "row",
                                   count / NUMBER_COLS,
                                   "column",
                                   count % NUMBER_COLS,
                                   NULL);
      g_debug (G_STRLOC ": Moving item (%s) to (%d, %d)",
               item->uuid,
               count / NUMBER_COLS,
               count % NUMBER_COLS);
    }
    count++;
  }
}

static void
_client_view_item_added_cb (MojitoClientView *view,
                         MojitoItem       *item,
                         gpointer          userdata)
{
  g_debug (G_STRLOC ": Item added: %s", item->uuid);
  penge_people_pane_update (userdata);
}

/* When we've been told something has been removed from the view then we
 * must remove it from our local hash of uuid to actor and then destroy the
 * actor. This should remove it from it's container, etc.
 *
 * We must then update the pane to ensure that we relayout the table.
 */
static void
_client_view_item_removed_cb (MojitoClientView *view,
                              MojitoItem       *item,
                              gpointer          userdata)
{
  PengePeoplePane *pane = PENGE_PEOPLE_PANE (userdata);
  PengePeoplePanePrivate *priv = GET_PRIVATE (pane);
  ClutterActor *actor;
 
  g_debug (G_STRLOC ": Item removed: %s", item->uuid);
  actor = g_hash_table_lookup (priv->uuid_to_actor,
                               item->uuid);

  g_hash_table_remove (priv->uuid_to_actor, item->uuid);
  clutter_container_remove_actor (CLUTTER_CONTAINER (view), actor);
  penge_people_pane_update (pane);
}


static void
_client_open_view_cb (MojitoClient     *client,
                      MojitoClientView *view,
                      gpointer          userdata)
{
  PengePeoplePane *pane = PENGE_PEOPLE_PANE (userdata);
  PengePeoplePanePrivate *priv = GET_PRIVATE (userdata);

  /* Save out the view */
  priv->view = g_object_ref (view);

  /* and start it ... */
  mojito_client_view_start (priv->view);

  g_signal_connect (priv->view, 
                    "item-added",
                    (GCallback)_client_view_item_added_cb,
                    pane);
/*
  g_signal_connect (priv->view, 
                    "item-changed",
                    _client_view_changed_cb,
                    object);
*/
  g_signal_connect (priv->view, 
                    "item-removed",
                    (GCallback)_client_view_item_removed_cb,
                    pane);
}

static void
_client_get_sources_cb (MojitoClient *client,
                        GList        *sources,
                        gpointer      userdata)
{
  GList *filtered_sources = NULL;
  GList *l;

  for (l = sources; l; l = l->next)
  {
    if (!g_str_equal (l->data, "dummy"))
    {
      filtered_sources = g_list_append (filtered_sources, l->data);
    }
  }

  mojito_client_open_view (client, 
                           filtered_sources, 
                           MAX_ITEMS, 
                           _client_open_view_cb, 
                           userdata);

  g_list_free (filtered_sources);
}

static void
penge_people_pane_class_init (PengePeoplePaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengePeoplePanePrivate));

  object_class->dispose = penge_people_pane_dispose;
  object_class->finalize = penge_people_pane_finalize;
}

static void
penge_people_pane_init (PengePeoplePane *self)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (self);
  NbtkPadding padding = { CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8) };

  priv->uuid_to_actor = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               g_free,
                                               g_object_unref);

  nbtk_table_set_row_spacing (NBTK_TABLE (self), 8);
  nbtk_table_set_col_spacing (NBTK_TABLE (self), 8);

  nbtk_widget_set_padding (NBTK_WIDGET (self), &padding);

  /* Create the client and request the sources list */
  priv->client = mojito_client_new ();
  mojito_client_get_sources (priv->client, _client_get_sources_cb, self);
}



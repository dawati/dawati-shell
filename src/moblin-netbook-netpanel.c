
#include <dbus/dbus-glib.h>
#include <clutter-mozembed.h>

#include "moblin-netbook-netpanel.h"
#include "moblin-netbook.h"
#include "mwb-radical-bar.h"

G_DEFINE_TYPE (MoblinNetbookNetpanel, moblin_netbook_netpanel, NBTK_TYPE_TABLE)

#define NETPANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MOBLIN_TYPE_NETBOOK_NETPANEL, MoblinNetbookNetpanelPrivate))

struct _MoblinNetbookNetpanelPrivate
{
  DBusGProxy     *proxy;
  GList          *calls;

  NbtkWidget     *tabs_table;
  gint            previews;

  NbtkWidget     *favs_table;
};

static void
moblin_netbook_netpanel_dispose (GObject *object)
{
  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (object)->priv;

  if (priv->proxy)
    {
      while (priv->calls)
        {
          DBusGProxyCall *call = priv->calls->data;
          dbus_g_proxy_cancel_call (priv->proxy, call);
          priv->calls = g_list_delete_link (priv->calls, priv->calls);
        }

      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }

  if (priv->tabs_table)
    {
      g_object_unref (G_OBJECT (priv->tabs_table));
      priv->tabs_table = NULL;
    }

  if (priv->favs_table)
    {
      g_object_unref (G_OBJECT (priv->favs_table));
      priv->favs_table = NULL;
    }

  G_OBJECT_CLASS (moblin_netbook_netpanel_parent_class)->dispose (object);
}

static void
moblin_netbook_netpanel_finalize (GObject *object)
{
  G_OBJECT_CLASS (moblin_netbook_netpanel_parent_class)->finalize (object);
}

static void
destroy_live_previews (MoblinNetbookNetpanel *self)
{
  GList *c, *children;

  MoblinNetbookNetpanelPrivate *priv = self->priv;

  /* Cancel any dbus calls */
  while (priv->calls)
    {
      DBusGProxyCall *call = priv->calls->data;
      dbus_g_proxy_cancel_call (priv->proxy, call);
      priv->calls = g_list_delete_link (priv->calls, priv->calls);
    }

  /* Destroy live previews */
  children =
    clutter_container_get_children (CLUTTER_CONTAINER (priv->tabs_table));
  for (c = children; c; c = c->next)
    {
      ClutterActor *child = c->data;

      if (CLUTTER_IS_MOZEMBED (child))
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->tabs_table),
                                        child);
    }
  g_list_free (children);
}

static void
notify_connect_view (DBusGProxy     *proxy,
                     DBusGProxyCall *call_id,
                     void           *user_data)
{
  GError *error = NULL;
  ClutterActor *mozembed = CLUTTER_ACTOR (user_data);
  MoblinNetbookNetpanel *self =
    g_object_get_data (G_OBJECT (mozembed), "netpanel");
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  priv->calls = g_list_remove (priv->calls, call_id);
  if (dbus_g_proxy_end_call (proxy, call_id, &error, G_TYPE_INVALID))
    {
      nbtk_table_add_actor_full (NBTK_TABLE (priv->tabs_table), mozembed,
                                 1, priv->previews, 1, 1,
                                 NBTK_KEEP_ASPECT_RATIO, 0.5, 0.5);
      priv->previews ++;

      /* Add the tabs table if this is the first preview we've received */
      if (priv->previews == 1)
        nbtk_table_add_widget_full (NBTK_TABLE (self), priv->tabs_table, 1, 0,
                                    1, 1, NBTK_X_EXPAND | NBTK_X_FILL,
                                    0.5, 0.5);
    }
  else
    {
      g_warning ("Error connecting tab: %s", error->message);
      g_error_free (error);
    }
}

static void
notify_get_ntabs (DBusGProxy     *proxy,
                  DBusGProxyCall *call_id,
                  void           *user_data)
{
  guint n_tabs;

  GError *error = NULL;
  MoblinNetbookNetpanel *self = MOBLIN_NETBOOK_NETPANEL (user_data);
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  priv->calls = g_list_remove (priv->calls, call_id);
  priv->previews = 0;
  if (dbus_g_proxy_end_call (proxy, call_id, &error, G_TYPE_UINT, &n_tabs,
                             G_TYPE_INVALID))
    {
      NbtkPadding padding;
      ClutterActor *parent;
      ClutterUnit cell_width;
      guint i;

      /* Calculate width of preview */
      /* We use the parent actor because we know we're contained in an
       * MnbDropDown with a constant width. This is horribly hacky though,
       * we should really just have an allocate function and not use table...
       */
      if ((parent = clutter_actor_get_parent (CLUTTER_ACTOR (self))))
        {
          nbtk_widget_get_padding (NBTK_WIDGET (self), &padding);
          cell_width = (clutter_actor_get_widthu (CLUTTER_ACTOR (parent)) -
                        padding.left - padding.right -
                        (nbtk_table_get_col_spacing (NBTK_TABLE (self)) * 4)) /
                       5.f;
        }
      else
        cell_width = 0;

      for (i = 0; i < n_tabs; i++)
        {
          gchar *input, *output;
          ClutterActor *mozembed;

          mozembed = clutter_mozembed_new_view ();
          clutter_actor_set_widthu (mozembed, cell_width);
          g_object_set_data (G_OBJECT (mozembed), "netpanel", self);
          g_object_get (G_OBJECT (mozembed),
                        "input", &input,
                        "output", &output,
                        NULL);

          priv->calls = g_list_prepend (priv->calls,
            dbus_g_proxy_begin_call (priv->proxy, "ConnectView",
                                     notify_connect_view,
                                     g_object_ref_sink (mozembed),
                                     g_object_unref,
                                     G_TYPE_UINT, i,
                                     G_TYPE_STRING, input,
                                     G_TYPE_STRING, output,
                                     G_TYPE_INVALID));

          g_free (input);
          g_free (output);
        }
    }
  else
    {
      /* TODO: Log the error if it's pertinent? */
      /*g_warning ("Error getting tabs: %s", error->message);*/
      g_error_free (error);
    }
}

static void
request_live_previews (MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  if (!priv->proxy)
    return;

  /* Get the number of tabs */
  priv->calls = g_list_prepend (priv->calls,
    dbus_g_proxy_begin_call (priv->proxy, "GetNTabs", notify_get_ntabs,
                             g_object_ref (self), g_object_unref,
                             G_TYPE_INVALID));
}

static void
moblin_netbook_netpanel_show (ClutterActor *actor)
{
  gint n_previews;
  GList *previews, *p;

  MoblinNetbookNetpanel *netpanel = MOBLIN_NETBOOK_NETPANEL (actor);
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;

  request_live_previews (netpanel);

  /* TODO: Favourites table */
  /*nbtk_table_add_widget_full (NBTK_TABLE (netpanel), priv->favs_table, 2, 0,
                              1, 1, NBTK_X_EXPAND | NBTK_X_FILL, 0.5, 0.5);*/

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->show (actor);
}

static void
moblin_netbook_netpanel_hide (ClutterActor *actor)
{
  GList *children, *c;

  MoblinNetbookNetpanel *netpanel = MOBLIN_NETBOOK_NETPANEL (actor);
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;

  /* Destroy live previews */
  destroy_live_previews (netpanel);

  /* Hide tabs/favs tables */
  clutter_container_remove_actor (CLUTTER_CONTAINER (netpanel),
                                  CLUTTER_ACTOR (priv->tabs_table));
  /*clutter_container_remove_actor (CLUTTER_CONTAINER (netpanel),
                                  CLUTTER_ACTOR (priv->favs_table));*/

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->hide (actor);
}

static void
moblin_netbook_netpanel_class_init (MoblinNetbookNetpanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MoblinNetbookNetpanelPrivate));

  object_class->dispose = moblin_netbook_netpanel_dispose;
  object_class->finalize = moblin_netbook_netpanel_finalize;

  actor_class->show = moblin_netbook_netpanel_show;
  actor_class->hide = moblin_netbook_netpanel_hide;
}

static void
moblin_netbook_netpanel_init (MoblinNetbookNetpanel *self)
{
  DBusGConnection *connection;
  NbtkWidget *table, *bar, *label, *more_button;

  GError *error = NULL;
  MoblinNetbookNetpanelPrivate *priv = self->priv = NETPANEL_PRIVATE (self);

  nbtk_table_set_col_spacing (NBTK_TABLE (self), 6);
  nbtk_table_set_row_spacing (NBTK_TABLE (self), 6);
  
  /* Construct internet panel (except tab/page previews) */
  
  /* Construct entry table */
  table = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (table), 6);
  nbtk_table_set_row_spacing (NBTK_TABLE (table), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (table), "netpanel-entrytable");

  nbtk_table_add_widget_full (NBTK_TABLE (self), table, 0, 0, 1, 1,
                              NBTK_X_EXPAND | NBTK_X_FILL, 0.5, 0.5);

  /* Construct entry table widgets */

  label = nbtk_label_new ("Internet");
  bar = mwb_radical_bar_new ();
  nbtk_table_add_widget_full (NBTK_TABLE (table), label, 0, 0, 1, 1,
                              0, 0.0, 0.5);
  nbtk_table_add_widget_full (NBTK_TABLE (table), bar, 0, 1, 1, 1,
                              NBTK_X_EXPAND | NBTK_X_FILL, 0.5, 0.5);

  /* Construct tabs preview table */
  priv->tabs_table = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (priv->tabs_table), 6);
  nbtk_table_set_row_spacing (NBTK_TABLE (priv->tabs_table), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (priv->tabs_table),
                          "netpanel-subtable");

  g_object_ref_sink (G_OBJECT (priv->tabs_table));

  /* Construct tabs previews table widgets */
  label = nbtk_label_new ("Tabs");
  nbtk_table_add_widget_full (NBTK_TABLE (priv->tabs_table), label, 0, 0, 1, 5,
                              0, 0.0, 0.5);

  /* Construct favourites preview table */
  priv->favs_table = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (priv->favs_table), 6);
  nbtk_table_set_row_spacing (NBTK_TABLE (priv->favs_table), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (priv->favs_table),
                          "netpanel-subtable");

  g_object_ref_sink (G_OBJECT (priv->favs_table));

  /* Construct favourites previews table widgets */
  label = nbtk_label_new ("Favourite pages");
  nbtk_table_add_widget_full (NBTK_TABLE (priv->favs_table), label, 0, 0, 1, 5,
                              0, 0.0, 0.5);

  /* Connect to DBus */
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection)
    {
      g_warning ("Failed to connect to session bus: %s", error->message);
      g_error_free (error);
    }
  else
    {
      priv->proxy = dbus_g_proxy_new_for_name (connection,
                                               "org.moblin.MoblinWebBrowser",
                                               "/org/moblin/MoblinWebBrowser",
                                               "org.moblin.MoblinWebBrowser");
    }
}

NbtkWidget*
moblin_netbook_netpanel_new (void)
{
  return g_object_new (MOBLIN_TYPE_NETBOOK_NETPANEL,
                       "visible", FALSE,
                       NULL);
}


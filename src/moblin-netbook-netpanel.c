
#include <dbus/dbus-glib.h>
#include <clutter-mozembed.h>

#include "moblin-netbook-netpanel.h"
#include "moblin-netbook.h"
#include "mwb-radical-bar.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (MoblinNetbookNetpanel, moblin_netbook_netpanel, NBTK_TYPE_TABLE)

#define NETPANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MOBLIN_TYPE_NETBOOK_NETPANEL, MoblinNetbookNetpanelPrivate))

enum
{
  LAUNCH,
  LAUNCHED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

struct _MoblinNetbookNetpanelPrivate
{
  DBusGProxy     *proxy;
  GList          *calls;

  NbtkWidget     *radical_bar;

  NbtkWidget     *tabs_table;
  NbtkWidget     *tabs_more;
  gint            previews;
};

static void
cancel_dbus_calls (MoblinNetbookNetpanel *self)
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
}

static void
moblin_netbook_netpanel_dispose (GObject *object)
{
  MoblinNetbookNetpanel *self = MOBLIN_NETBOOK_NETPANEL (object);
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  if (priv->proxy)
    {
      cancel_dbus_calls (self);
      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }

  G_OBJECT_CLASS (moblin_netbook_netpanel_parent_class)->dispose (object);
}

static void
moblin_netbook_netpanel_finalize (GObject *object)
{
  G_OBJECT_CLASS (moblin_netbook_netpanel_parent_class)->finalize (object);
}

static void
tabs_more_clicked_cb (NbtkWidget *button, MoblinNetbookNetpanel *self)
{
  g_signal_emit (self, signals[LAUNCH], 0, "");
}

static void
create_tabs_table (MoblinNetbookNetpanel *self)
{
  NbtkWidget *label;

  MoblinNetbookNetpanelPrivate *priv = self->priv;

  /* Construct tabs preview table */
  priv->tabs_table = nbtk_table_new ();

  nbtk_table_add_widget_full (NBTK_TABLE (self), priv->tabs_table, 1, 0,
                              1, 1, NBTK_X_EXPAND | NBTK_X_FILL,
                              0.5, 0.5);

  nbtk_table_set_col_spacing (NBTK_TABLE (priv->tabs_table), 6);
  nbtk_table_set_row_spacing (NBTK_TABLE (priv->tabs_table), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (priv->tabs_table),
                          "netpanel-subtable");

  /* Construct tabs previews table widgets */
  label = nbtk_label_new (_("Tabs"));
  nbtk_table_add_widget_full (NBTK_TABLE (priv->tabs_table), label, 0, 0, 1, 5,
                              0, 0.0, 0.5);
  priv->tabs_more = nbtk_button_new_with_label (_("More..."));
  nbtk_table_add_widget_full (NBTK_TABLE (priv->tabs_table), priv->tabs_more,
                              1, 5, 1, 1, 0, 0.5, 0.5);
  clutter_actor_hide (CLUTTER_ACTOR (priv->tabs_more));

  g_signal_connect (priv->tabs_more, "clicked",
                    G_CALLBACK (tabs_more_clicked_cb), self);
}

static void
mozembed_button_clicked_cb (NbtkBin *button, MoblinNetbookNetpanel *self)
{
  guint tab;

  MoblinNetbookNetpanelPrivate *priv = self->priv;
  ClutterActor *mozembed = nbtk_bin_get_child (button);

  tab = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (mozembed), "tab"));

  /* FIXME: Check if dbus_g_proxy_call_no_reply is async... Gotta love
   *        those docs!
   */
  dbus_g_proxy_call_no_reply (priv->proxy, "SwitchTab", G_TYPE_UINT, tab,
                              G_TYPE_INVALID);
  dbus_g_proxy_call_no_reply (priv->proxy, "Raise", G_TYPE_INVALID);

  g_signal_emit (self, signals[LAUNCHED], 0);
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
      NbtkWidget *button;

      button = nbtk_button_new ();
      clutter_container_add_actor (CLUTTER_CONTAINER (button), mozembed);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (mozembed_button_clicked_cb), self);
      nbtk_table_add_widget_full (NBTK_TABLE (priv->tabs_table), button,
                                  1, priv->previews, 1, 1,
                                  NBTK_KEEP_ASPECT_RATIO, 0.5, 0.5);
      priv->previews ++;
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
  if (!dbus_g_proxy_end_call (proxy, call_id, &error, G_TYPE_UINT, &n_tabs,
                              G_TYPE_INVALID))
    {
      /* TODO: Log the error if it's pertinent? */
      /*g_warning ("Error getting tabs: %s", error->message);*/
      g_error_free (error);
      n_tabs = 0;
    }

  if (n_tabs)
    {
      NbtkPadding padding;
      ClutterActor *parent;
      ClutterUnit cell_width;
      guint i;

      /* Create tabs table */
      create_tabs_table (self);

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

      for (i = 0; i < MIN (4, n_tabs); i++)
        {
          gchar *input, *output;
          ClutterActor *mozembed;

          mozembed = clutter_mozembed_new_view ();
          clutter_actor_set_widthu (mozembed, cell_width);
          clutter_actor_set_reactive (mozembed, FALSE);
          g_object_set_data (G_OBJECT (mozembed), "netpanel", self);
          g_object_set_data (G_OBJECT (mozembed), "tab", GUINT_TO_POINTER (i));
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

      if (n_tabs > 4)
        clutter_actor_show (CLUTTER_ACTOR (priv->tabs_more));
    }
  else
    {
      /* Add a placeholder text label */
      priv->tabs_table = nbtk_label_new (_("Type an address or load the browser "
                                         "to see your tabs here."));
      clutter_actor_set_name (CLUTTER_ACTOR (priv->tabs_table),
                              "netpanel-placeholder");
      nbtk_table_add_widget_full (NBTK_TABLE (self), priv->tabs_table, 1, 0,
                                  1, 1, 0, 0.5, 0.5);
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

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->show (actor);
}

static void
moblin_netbook_netpanel_hide (ClutterActor *actor)
{
  GList *children, *c;

  MoblinNetbookNetpanel *netpanel = MOBLIN_NETBOOK_NETPANEL (actor);
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;

  /* Clear the entry */
  mwb_radical_bar_set_text (MWB_RADICAL_BAR (priv->radical_bar), "");
  mwb_radical_bar_set_loading (MWB_RADICAL_BAR (priv->radical_bar), FALSE);

  /* Destroy tab table */
  cancel_dbus_calls (netpanel);
  if (priv->tabs_table)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (netpanel),
                                      CLUTTER_ACTOR (priv->tabs_table));
      priv->tabs_table = NULL;
      priv->tabs_more = NULL;
    }

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

  signals[LAUNCH] =
    g_signal_new ("launch",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MoblinNetbookNetpanelClass, launch),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  signals[LAUNCHED] =
    g_signal_new ("launched",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MoblinNetbookNetpanelClass, launched),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
radical_bar_go_cb (MwbRadicalBar         *radical_bar,
                   const gchar           *url,
                   MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  if (url && (url[0] != '\0'))
    {
      g_signal_emit (self, signals[LAUNCH], 0, url);
      mwb_radical_bar_set_loading (radical_bar, TRUE);
    }
}

static void
moblin_netbook_netpanel_init (MoblinNetbookNetpanel *self)
{
  DBusGConnection *connection;
  NbtkWidget *table, *label, *more_button;

  GError *error = NULL;
  MoblinNetbookNetpanelPrivate *priv = self->priv = NETPANEL_PRIVATE (self);

  nbtk_table_set_col_spacing (NBTK_TABLE (self), 6);
  nbtk_table_set_row_spacing (NBTK_TABLE (self), 6);
  
  /* Construct entry table */
  table = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (table), 6);
  nbtk_table_set_row_spacing (NBTK_TABLE (table), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (table), "netpanel-entrytable");

  nbtk_table_add_widget_full (NBTK_TABLE (self), table, 0, 0, 1, 1,
                              NBTK_X_EXPAND | NBTK_X_FILL |
                              NBTK_Y_EXPAND | NBTK_Y_FILL, 0.5, 0.5);

  /* Construct entry table widgets */

  label = nbtk_label_new (_("Internet"));
  priv->radical_bar = mwb_radical_bar_new ();
  nbtk_table_add_widget_full (NBTK_TABLE (table), label, 0, 0, 1, 1,
                              0, 0.0, 0.5);
  nbtk_table_add_widget_full (NBTK_TABLE (table), priv->radical_bar, 0, 1, 1, 1,
                              NBTK_X_EXPAND | NBTK_X_FILL |
                              NBTK_Y_EXPAND | NBTK_Y_FILL, 0.5, 0.5);
  g_signal_connect (priv->radical_bar, "go",
                    G_CALLBACK (radical_bar_go_cb), self);

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

void
moblin_netbook_netpanel_focus (MoblinNetbookNetpanel *netpanel)
{
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;
  mwb_radical_bar_focus (MWB_RADICAL_BAR (priv->radical_bar));
}

void
moblin_netbook_netpanel_clear (MoblinNetbookNetpanel *netpanel)
{
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;
  mwb_radical_bar_set_text (MWB_RADICAL_BAR (priv->radical_bar), "");
}


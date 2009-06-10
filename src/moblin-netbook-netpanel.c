
#include <dbus/dbus-glib.h>
#include <clutter-mozembed.h>

#include "moblin-netbook-netpanel.h"
#include "moblin-netbook.h"

#include <glib/gi18n.h>

/*
  It wasn't clear to me, whether we might go back to using the radical bar
  later, with its additional features like the dropdown search list, so I put
  in #ifs while changing to use MnbEntry. -- Geoff
 */
#define USE_RADICAL_BAR 0

#if USE_RADICAL_BAR
#include "mwb-radical-bar.h"
#else
#include "mnb-entry.h"
#endif

/* number of tab columns to display */
#define DISPLAY_TABS 4

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

  NbtkWidget     *entry;

  NbtkWidget     *tabs_table;
  NbtkWidget     *tabs_prev;
  NbtkWidget     *tabs_next;

  guint           n_tabs;
  guint           display_tab;

  NbtkWidget    **tabs;
  NbtkWidget    **labels;
};

static void
cancel_dbus_calls (MoblinNetbookNetpanel *self)
{
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
new_tab_clicked_cb (NbtkWidget *button, MoblinNetbookNetpanel *self)
{
  /* FIXME: remove hardcoded start path */
  g_signal_emit (self, signals[LAUNCH], 0,
                 "chrome://startpage/content/index.html");
}

static void
get_tab_display_range (MoblinNetbookNetpanel *self, guint *start, guint *end)
{
  /* returns: range of tabs currently displayed: includes start, excludes end */
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);

  *start = priv->display_tab;
  *end = *start + DISPLAY_TABS;

  if (*end > priv->n_tabs)
    {
      *end = priv->n_tabs;
      if (*end >= DISPLAY_TABS)
        *start = *end - DISPLAY_TABS;
      else
        *start = 0;
    }
}

static void
redisplay_tabs (MoblinNetbookNetpanel *self, guint old_start, guint old_end,
                guint new_start, guint new_end)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;
  guint i;

  for (i = old_start; i < old_end; i++)
    {
      if (priv->tabs[i])
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->tabs_table),
                                        CLUTTER_ACTOR (priv->tabs[i]));
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->tabs_table),
                                      CLUTTER_ACTOR (priv->labels[i]));
    }

  for (i = new_start; i < new_end; i++)
    {
      if (priv->tabs[i])
        nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table),
                                              CLUTTER_ACTOR (priv->tabs[i]),
                                              1, i - new_start,
                                              "x-expand", FALSE,
                                              "y-expand", FALSE,
                                              "x-fill", FALSE,
                                              "y-fill", FALSE,
                                              "x-align", 0.5,
                                              "y-align", 0.5,
                                              NULL);
      nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table),
                                            CLUTTER_ACTOR (priv->labels[i]),
                                            2, i - new_start,
                                            "x-expand", FALSE,
                                            "y-expand", FALSE,
                                            "x-fill", FALSE,
                                            "y-fill", FALSE,
                                            "x-align", 0.0,
                                            "y-align", 0.5,
                                            NULL);
    }
}

static void
tabs_prev_clicked_cb (NbtkWidget *button, MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;
  guint old_start, old_end, new_start, new_end;

  get_tab_display_range (self, &old_start, &old_end);
  if (priv->display_tab >= DISPLAY_TABS)
    priv->display_tab -= DISPLAY_TABS;
  else
    priv->display_tab = 0;
  get_tab_display_range (self, &new_start, &new_end);

  if (old_start == new_start)
    return;

  redisplay_tabs (self, old_start, old_end, new_start, new_end);

  if (!priv->display_tab)
    {
      /* workaround to make sure button isn't frozen in active state */
      nbtk_widget_set_style_pseudo_class (priv->tabs_prev, NULL);

      clutter_actor_hide (CLUTTER_ACTOR (priv->tabs_prev));
    }
  clutter_actor_show (CLUTTER_ACTOR (priv->tabs_next));
}

static void
tabs_next_clicked_cb (NbtkWidget *button, MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;
  guint old_start, old_end, new_start, new_end;

  get_tab_display_range (self, &old_start, &old_end);
  priv->display_tab += DISPLAY_TABS;
  get_tab_display_range (self, &new_start, &new_end);

  if (old_start == new_start)
    return;

  priv->display_tab = new_start;

  redisplay_tabs (self, old_start, old_end, new_start, new_end);

  clutter_actor_show (CLUTTER_ACTOR (priv->tabs_prev));
  if (priv->display_tab + DISPLAY_TABS >= priv->n_tabs)
    {
      /* workaround to make sure button isn't frozen in active state */
      nbtk_widget_set_style_pseudo_class (priv->tabs_next, NULL);

      clutter_actor_hide (CLUTTER_ACTOR (priv->tabs_next));
    }
}

static void
create_tabs_table (MoblinNetbookNetpanel *self)
{
  NbtkWidget *label, *bin;

  MoblinNetbookNetpanelPrivate *priv = self->priv;

  /* Construct tabs preview table */
  priv->tabs_table = nbtk_table_new ();

  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        CLUTTER_ACTOR (priv->tabs_table),
                                        1, 0,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", FALSE,
                                        "x-align", 0.5,
                                        "y-align", 0.5,
                                        NULL);

  nbtk_table_set_col_spacing (NBTK_TABLE (priv->tabs_table), 6);
  nbtk_table_set_row_spacing (NBTK_TABLE (priv->tabs_table), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (priv->tabs_table),
                          "netpanel-subtable");

  /* Construct tabs previews table widgets */
  label = nbtk_label_new (_("Tabs"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "section");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table),
                                        CLUTTER_ACTOR (label),
                                        0, 0,
                                        "row-span", 1,
                                        "col-span", DISPLAY_TABS + 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

  /* Create filler element so table expands for the placeholder text */
  label = nbtk_label_new ("");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table),
                                        CLUTTER_ACTOR (label),
                                        1, DISPLAY_TABS,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        NULL);

  label = nbtk_label_new (_("Favorite pages"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "section");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table),
                                        CLUTTER_ACTOR (label),
                                        4, 0,
                                        "row-span", 1,
                                        "col-span", DISPLAY_TABS + 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

  bin = nbtk_bin_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "netpanel-placeholder-bin");

  label = nbtk_label_new (_("As you visit web pages, your favorites will "
                            "appear here and on the New tab page in the "
                            "browser."));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "netpanel-placeholder-label");
  nbtk_bin_set_child (NBTK_BIN (bin), CLUTTER_ACTOR (label));
  nbtk_bin_set_alignment (NBTK_BIN (bin), NBTK_ALIGN_LEFT, NBTK_ALIGN_CENTER);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table),
                                        CLUTTER_ACTOR (bin),
                                        5, 0,
                                        "row-span", 1,
                                        "col-span", DISPLAY_TABS + 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);
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
      guint tab, start, end;

      tab = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (mozembed), "tab"));

      priv->tabs[tab] = button = nbtk_button_new ();
      g_object_ref_sink (priv->tabs[tab]);

      clutter_container_add_actor (CLUTTER_CONTAINER (button), mozembed);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (mozembed_button_clicked_cb), self);

      get_tab_display_range (self, &start, &end);
      if (tab >= start && tab < end)
        nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table),
                                              CLUTTER_ACTOR (button),
                                              1, tab - start,
                                              "x-expand", FALSE,
                                              "y-expand", FALSE,
                                              "x-fill", FALSE,
                                              "y-fill", FALSE,
                                              "x-align", 0.5,
                                              "y-align", 0.5,
                                              NULL);
    }
  else
    {
      g_warning ("Error connecting tab: %s", error->message);
      g_error_free (error);
    }
}

static void
notify_get_tab (DBusGProxy     *proxy,
                DBusGProxyCall *call_id,
                void           *user_data)
{
  gchar *url = NULL, *title = NULL;
  guint tab, start, end;

  GError *error = NULL;
  ClutterActor *label = CLUTTER_ACTOR (user_data);
  MoblinNetbookNetpanel *self =
    g_object_get_data (G_OBJECT (label), "netpanel");
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  priv->calls = g_list_remove (priv->calls, call_id);
  if (!dbus_g_proxy_end_call (proxy, call_id, &error,
                              G_TYPE_STRING, &url,
                              G_TYPE_STRING, &title,
                              G_TYPE_INVALID))
    {
      /* TODO: Log the error if it's pertinent? */
      g_warning ("Error getting tab info: %s", error->message);
      g_error_free (error);
      g_free (url);
      g_free (title);
      return;
    }

  tab = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (label), "tab"));

  nbtk_label_set_text (NBTK_LABEL (label),
                       title && (title[0] != '\0') ? title : _("Untitled"));

  get_tab_display_range (self, &start, &end);
  if (tab >= start && tab < end)
    nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table), label,
                                          2, tab - start,
                                          "x-expand", FALSE,
                                          "y-expand", FALSE,
                                          "x-fill", FALSE,
                                          "y-fill", FALSE,
                                          "x-align", 0.0,
                                          "y-align", 0.5,
                                          NULL);

  g_free (url);
  g_free (title);
}

static void
notify_get_ntabs (DBusGProxy     *proxy,
                  DBusGProxyCall *call_id,
                  void           *user_data)
{
  NbtkPadding padding;
  gfloat cell_width;
  NbtkWidget *label;
  ClutterActor *parent;

  GError *error = NULL;
  MoblinNetbookNetpanel *self = MOBLIN_NETBOOK_NETPANEL (user_data);
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  priv->display_tab = 0;

  /* Create tabs table */
  create_tabs_table (self);

  priv->calls = g_list_remove (priv->calls, call_id);
  if (!dbus_g_proxy_end_call (proxy, call_id, &error, G_TYPE_UINT,
                              &priv->n_tabs, G_TYPE_INVALID))
    {
      /* TODO: Log the error if it's pertinent? */
      g_warning ("Error getting tabs: %s", error->message);
      g_error_free (error);
      priv->n_tabs = 0;
    }

  /* Calculate width of preview */
  /* We use the parent actor because we know we're contained in an
   * MnbDropDown with a constant width. This is horribly hacky though,
   * we should really just have an allocate function and not use table...
   */
  if ((parent = clutter_actor_get_parent (CLUTTER_ACTOR (self))))
    {
      nbtk_widget_get_padding (NBTK_WIDGET (self), &padding);
      cell_width = (clutter_actor_get_width (CLUTTER_ACTOR (parent)) -
                    padding.left - padding.right -
                    (nbtk_table_get_col_spacing (NBTK_TABLE (self))
                     * DISPLAY_TABS)) / (1.0f * (DISPLAY_TABS + 1));
    }
  else
    cell_width = 0;

  if (priv->n_tabs)
    {
      guint i;

      priv->tabs = (NbtkWidget **)g_malloc0 (sizeof (NbtkWidget *) *
                                             priv->n_tabs);
      priv->labels = (NbtkWidget **)g_malloc0 (sizeof (NbtkWidget *) *
                                               priv->n_tabs);

      for (i = 0; i < priv->n_tabs; i++)
        {
          gchar *input, *output;
          ClutterActor *mozembed;

          mozembed = clutter_mozembed_new_view ();
          clutter_actor_set_width (mozembed, cell_width);
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

          priv->labels[i] = label = nbtk_label_new ("");
          g_object_ref (label);

          clutter_actor_set_width (CLUTTER_ACTOR (label), cell_width);
          g_object_set_data (G_OBJECT (label), "netpanel", self);
          g_object_set_data (G_OBJECT (label), "tab", GUINT_TO_POINTER (i));

          priv->calls = g_list_prepend (priv->calls,
            dbus_g_proxy_begin_call (priv->proxy, "GetTab",
                                     notify_get_tab,
                                     g_object_ref_sink (label),
                                     g_object_unref,
                                     G_TYPE_UINT, i,
                                     G_TYPE_INVALID));

          g_free (input);
          g_free (output);
        }
    }
  else
    {
      NbtkWidget *button;
      gfloat cell_height;

      /* FIXME: this should be a default image or startpage snapshot instead,
                and this label should become the caption title */
      label = nbtk_label_new (_("New tab"));

      /* FIXME: this is just a guess at aspect ratio */
      cell_height = cell_width * 9.0 / 16.0;

      button = nbtk_button_new ();
      clutter_actor_set_size (CLUTTER_ACTOR (button), cell_width, cell_height);
      clutter_container_add_actor (CLUTTER_CONTAINER (button),
                                   CLUTTER_ACTOR (label));
      g_signal_connect (button, "clicked",
                        G_CALLBACK (new_tab_clicked_cb), self);
      nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table),
                                            CLUTTER_ACTOR (button),
                                            1, 0,
                                            "x-expand", FALSE,
                                            "y-expand", FALSE,
                                            "x-fill", FALSE,
                                            "y-fill", FALSE,
                                            "x-align", 0.0,
                                            "y-align", 0.5,
                                            NULL);

      priv->n_tabs = 1;
    }

  /* FIXME: try to load favorites pages here */

  if (priv->n_tabs > DISPLAY_TABS)
    {
      priv->tabs_prev = nbtk_button_new_with_label (_("Previous"));
      nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table),
                                            CLUTTER_ACTOR (priv->tabs_prev),
                                            6, 0,
                                            "x-expand", FALSE,
                                            "y-expand", FALSE,
                                            "x-fill", FALSE,
                                            "y-fill", FALSE,
                                            "x-align", 0.0,
                                            "y-align", 0.5,
                                            NULL);
      clutter_actor_hide (CLUTTER_ACTOR (priv->tabs_prev));
      g_signal_connect (priv->tabs_prev, "clicked",
                        G_CALLBACK (tabs_prev_clicked_cb), self);

      priv->tabs_next = nbtk_button_new_with_label (_("Next"));
      nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->tabs_table),
                                            CLUTTER_ACTOR (priv->tabs_next),
                                            6, 3,
                                            "x-expand", FALSE,
                                            "y-expand", FALSE,
                                            "x-fill", FALSE,
                                            "y-fill", FALSE,
                                            "x-align", 1.0,
                                            "y-align", 0.5,
                                            NULL);
      g_signal_connect (priv->tabs_next, "clicked",
                        G_CALLBACK (tabs_next_clicked_cb), self);
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
  MoblinNetbookNetpanel *netpanel = MOBLIN_NETBOOK_NETPANEL (actor);

  request_live_previews (netpanel);

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->show (actor);
}

static void
moblin_netbook_netpanel_hide (ClutterActor *actor)
{
  MoblinNetbookNetpanel *netpanel = MOBLIN_NETBOOK_NETPANEL (actor);
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;
  guint i;

  /* Clear the entry */
#if USE_RADICAL_BAR
  mwb_radical_bar_set_text (MWB_RADICAL_BAR (priv->entry), "");
  mwb_radical_bar_set_loading (MWB_RADICAL_BAR (priv->entry), FALSE);
#else
  mnb_entry_set_text  (MNB_ENTRY (priv->entry), "");
#endif

  /* Destroy tab table */
  cancel_dbus_calls (netpanel);
  if (priv->tabs_table)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (netpanel),
                                      CLUTTER_ACTOR (priv->tabs_table));
      priv->tabs_table = NULL;
      priv->tabs_prev = NULL;
      priv->tabs_next = NULL;
    }

  if (priv->tabs)
    {
      for (i = 0; i < priv->n_tabs; i++)
        if (priv->tabs[i])
          g_object_unref (priv->tabs[i]);
      g_free (priv->tabs);
      priv->tabs = NULL;
    }

  if (priv->labels)
    {
      for (i = 0; i < priv->n_tabs; i++)
        g_object_unref (priv->labels[i]);
      g_free (priv->labels);
      priv->labels = NULL;
    }

  priv->n_tabs = 0;

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

#if USE_RADICAL_BAR

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

#else

static void
entry_button_clicked_cb (MnbEntry              *entry,
                         MoblinNetbookNetpanel *self)
{
  const gchar *url = mnb_entry_get_text (entry);

  if (url)
    {
      while (*url == ' ')
        url++;
      if (*url == '\0')
        mnb_entry_set_text (entry, "");
      else
        g_signal_emit (self, signals[LAUNCH], 0, url);
    }
}

static void
entry_keynav_event_cb (MnbEntry              *entry,
                       guint                  nav,
                       MoblinNetbookNetpanel *self)
{
  if (nav == CLUTTER_Return)
    entry_button_clicked_cb (entry, self);
}

#endif

static void
moblin_netbook_netpanel_init (MoblinNetbookNetpanel *self)
{
  DBusGConnection *connection;
  NbtkWidget *table, *label;

  GError *error = NULL;
  MoblinNetbookNetpanelPrivate *priv = self->priv = NETPANEL_PRIVATE (self);

  nbtk_table_set_col_spacing (NBTK_TABLE (self), 6);
  nbtk_table_set_row_spacing (NBTK_TABLE (self), 6);

  /* Construct entry table */
  table = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (table), 6);
  nbtk_table_set_row_spacing (NBTK_TABLE (table), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (table), "netpanel-entrytable");

  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        CLUTTER_ACTOR (table),
                                        0, 0,
                                        "row-span", 1,
                                        "col-span", 1,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        NULL);

  /* Construct entry table widgets */

  label = nbtk_label_new (_("Internet"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "netpanel-label");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (table),
                                        CLUTTER_ACTOR (label),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

#if USE_RADICAL_BAR
  priv->entry = mwb_radical_bar_new ();
  clutter_actor_set_height (CLUTTER_ACTOR (priv->entry), 36);
#else
  priv->entry = mnb_entry_new (_("Go"));
#endif
  clutter_actor_set_name (CLUTTER_ACTOR (priv->entry), "netpanel-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (priv->entry), 600);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (table),
                                        CLUTTER_ACTOR (priv->entry),
                                        0, 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

#if USE_RADICAL_BAR
  g_signal_connect (priv->entry, "go",
                    G_CALLBACK (radical_bar_go_cb), self);
#else
  g_signal_connect (priv->entry, "button-clicked",
                    G_CALLBACK (entry_button_clicked_cb), self);
  g_signal_connect (priv->entry, "keynav-event",
                    G_CALLBACK (entry_keynav_event_cb), self);
#endif

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
#if USE_RADICAL_BAR
  mwb_radical_bar_focus (MWB_RADICAL_BAR (priv->entry));
#else
  clutter_actor_grab_key_focus (CLUTTER_ACTOR (priv->entry));
#endif
}

void
moblin_netbook_netpanel_clear (MoblinNetbookNetpanel *netpanel)
{
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;
#if USE_RADICAL_BAR
  mwb_radical_bar_set_text (MWB_RADICAL_BAR (priv->entry), "");
#else
  mnb_entry_set_text (MNB_ENTRY (priv->entry), "");
#endif
}

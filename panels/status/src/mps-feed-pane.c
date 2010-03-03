/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Rob bradford <rob@linux.intel.com>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <clutter/x11/clutter-x11.h>

#include <libsocialweb-client/sw-client.h>
#include <libsocialweb-client/sw-client-service.h>
#include <mx/mx.h>

#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-entry.h>
#include <moblin-panel/mpl-panel-common.h>

#include "mps-view-bridge.h"
#include "mps-feed-pane.h"
#include "mps-tweet-card.h"
#include "mps-geotag-pane.h"

G_DEFINE_TYPE (MpsFeedPane, mps_feed_pane, MX_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPS_TYPE_FEED_PANE, MpsFeedPanePrivate))

typedef struct _MpsFeedPanePrivate MpsFeedPanePrivate;

struct _MpsFeedPanePrivate {
  SwClient *client;
  SwClientService *service;
  SwClientView *view;
  MpsViewBridge *bridge;

  ClutterActor *update_hbox;
  ClutterActor *entry;
  ClutterActor *update_button;

  ClutterActor *something_wrong_frame;
  ClutterActor *something_wrong_label;

  ClutterActor *scroll_view;
  ClutterActor *box_layout;

  ClutterActor *progress_label;

  ClutterActor *location_hbox;
  ClutterActor *main_notebook;
  ClutterActor *geotag_pane;
  ClutterActor *location_button;
  ClutterActor *location_label;
};

enum
{
  PROP_0,
  PROP_CLIENT,
  PROP_SERVICE
};

static void
mps_feed_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  MpsFeedPanePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CLIENT:
      g_value_set_object (value, priv->client);
      break;
    case PROP_SERVICE:
      g_value_set_object (value, priv->service);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_feed_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  MpsFeedPanePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CLIENT:
      priv->client = g_value_dup_object (value);
      break;
    case PROP_SERVICE:
      priv->service = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_feed_pane_dispose (GObject *object)
{
  MpsFeedPanePrivate *priv = GET_PRIVATE (object);

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }

  if (priv->service)
  {
    g_object_unref (priv->service);
    priv->service = NULL;
  }

  if (priv->view)
  {
    g_object_unref (priv->view);
    priv->view = NULL;
  }

  if (priv->bridge)
  {
    g_object_unref (priv->bridge);
    priv->bridge = NULL;
  }

  G_OBJECT_CLASS (mps_feed_pane_parent_class)->dispose (object);
}

static void
mps_feed_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (mps_feed_pane_parent_class)->finalize (object);
}

static void
_client_view_opened_cb (SwClient     *client,
                        SwClientView *view,
                        gpointer      userdata) 
{
  MpsFeedPane *pane = MPS_FEED_PANE (userdata);
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);

  priv->view = g_object_ref (view);

  mps_view_bridge_set_view (priv->bridge, view);

  g_object_unref (pane);
}

static void
_service_status_updated_cb (SwClient *service,
                            gboolean  success,
                            gpointer  userdata)
{
  MpsFeedPane *pane = MPS_FEED_PANE (userdata);
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);

  if (success)
  {
    ClutterActor *entry;
    sw_client_view_refresh (priv->view);
    entry = (ClutterActor *)mpl_entry_get_mx_entry (MPL_ENTRY (priv->entry));
    mx_entry_set_text (MX_ENTRY (entry), NULL);
  }
}

static gboolean
_has_cap (const gchar **caps,
          const gchar *cap)
{
  if (!caps)
    return FALSE;

  while (*caps)
  {
    if (g_str_equal (*caps, cap))
      return TRUE;

    caps++;
  }

  return FALSE;
}

static void
_update_from_caps (MpsFeedPane  *pane,
                   const gchar **caps)
{
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);

  if (_has_cap (caps, "can-update-status"))
  {
    clutter_actor_hide (priv->something_wrong_frame);
    clutter_actor_show (priv->update_hbox);
  } else {
    clutter_actor_hide (priv->update_hbox);
    clutter_actor_show (priv->something_wrong_frame);
  }
}

static void
_service_capabilities_changed_cb (SwClientService  *service,
                                  const gchar     **caps,
                                  gpointer          userdata)
{
  _update_from_caps (MPS_FEED_PANE (userdata), caps);
}

static void
_service_get_dynamic_caps_cb (SwClientService  *service,
                              const gchar     **caps,
                              const GError     *error,
                              gpointer          userdata)
{
  _update_from_caps (MPS_FEED_PANE (userdata), caps);
}

static void
mps_feed_pane_constructed (GObject *object)
{
  MpsFeedPane *pane = MPS_FEED_PANE (object);
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);
  const gchar *service_name;

  service_name = sw_client_service_get_name (priv->service);

  g_signal_connect (priv->service,
                    "status-updated",
                    (GCallback)_service_status_updated_cb,
                    pane);
  g_signal_connect (priv->service,
                    "capabilities-changed",
                    (GCallback)_service_capabilities_changed_cb,
                    pane);
  sw_client_service_get_dynamic_capabilities (priv->service,
                                              _service_get_dynamic_caps_cb,
                                              pane);

  sw_client_open_view_for_service (priv->client,
                                   service_name,
                                   20,
                                   _client_view_opened_cb,
                                   g_object_ref (pane));

  if (G_OBJECT_CLASS (mps_feed_pane_parent_class)->constructed)
  {
    G_OBJECT_CLASS (mps_feed_pane_parent_class)->constructed (object);
  }
}

static void
mps_feed_pane_class_init (MpsFeedPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MpsFeedPanePrivate));

  object_class->get_property = mps_feed_pane_get_property;
  object_class->set_property = mps_feed_pane_set_property;
  object_class->dispose = mps_feed_pane_dispose;
  object_class->finalize = mps_feed_pane_finalize;
  object_class->constructed = mps_feed_pane_constructed;

  pspec = g_param_spec_object ("client",
                               "client",
                               "The client-side core",
                               SW_TYPE_CLIENT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_CLIENT, pspec);

  pspec = g_param_spec_object ("service",
                               "service",
                               "The client-side service object",
                               SW_CLIENT_TYPE_SERVICE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_SERVICE, pspec);
}

static void
_service_update_status_cb (SwClientService *service,
                           const GError        *error,
                           gpointer             userdata)
{
  MpsFeedPane *pane = MPS_FEED_PANE (userdata);
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);

  if (error)
  {
    g_warning (G_STRLOC ": Error updating status: %s",
               error->message);
  }
}

static void
_update_button_clicked_cb (MplEntry    *entry,
                           MpsFeedPane *pane)
{
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);
  const gchar *status_message;

  status_message = mpl_entry_get_text (MPL_ENTRY (priv->entry));

  sw_client_service_update_status (priv->service,
                                   _service_update_status_cb,
                                   status_message,
                                   pane);
}

static void
_entry_activate_cb (ClutterText *text,
                    MpsFeedPane *pane)
{
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);
  const gchar *status_message;

  status_message = mpl_entry_get_text (MPL_ENTRY (priv->entry));

  sw_client_service_update_status (priv->service,
                                   _service_update_status_cb,
                                   status_message,
                                   pane);
}


static void
_update_location_label (MpsFeedPane *pane)
{
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);
  gboolean geotag_enabled, guess_location;
  gdouble latitude, longitude;

  g_object_get (priv->geotag_pane,
                "geotag-enabled", &geotag_enabled,
                "guess-location", &guess_location,
                "latitude", &latitude,
                "longitude", &longitude,
                NULL);

  if (geotag_enabled)
  {
    gchar *message;

    if (guess_location)
    {
      message = g_strdup_printf (_("We guessed your location as: %f %f"),
                                   latitude,
                                   longitude);
    } else {
      message = g_strdup_printf (_("Your location is being shared as: %f %f "),
                                   latitude,
                                   longitude);
    }
    mx_label_set_text (MX_LABEL (priv->location_label), message);
    g_free (message);
  } else {
    mx_label_set_text (MX_LABEL (priv->location_label), _("Your location is not being shared"));
  }
}

static void
_location_button_clicked_cb (MxButton    *button,
                             MpsFeedPane *pane)
{
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);

  clutter_actor_hide (priv->scroll_view);
  clutter_actor_show (priv->geotag_pane);
}

static void
_geotag_pane_location_chosen (MpsGeotagPane *geotag_pane,
                              MpsFeedPane   *pane)
{
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);

  _update_location_label (pane);

  clutter_actor_hide (priv->geotag_pane);
  clutter_actor_show (priv->scroll_view);
}

static void
_card_reply_clicked (MpsTweetCard *card,
                     gpointer      userdata)
{
  MpsFeedPane *pane = MPS_FEED_PANE (userdata);
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);
  SwItem *item;
  gchar *reply_msg;

  item = mps_tweet_card_get_item (card);

  reply_msg = g_strdup_printf ("@%s ",
                               sw_item_get_value (item, "screen_name"));
  mpl_entry_set_text (MPL_ENTRY (priv->entry), reply_msg);
  clutter_actor_grab_key_focus (priv->entry);
  g_free (reply_msg);
}

static void
_card_retweet_clicked (MpsTweetCard *card,
                       gpointer      userdata)
{
  MpsFeedPane *pane = MPS_FEED_PANE (userdata);
  MpsFeedPanePrivate *priv = GET_PRIVATE (pane);
  SwItem *item;
  gchar *retweet_msg;

  item = mps_tweet_card_get_item (card);

  retweet_msg = g_strdup_printf ("RT @%s: %s",
                                 sw_item_get_value (item, "screen_name"),
                                 sw_item_get_value (item, "content"));

  mpl_entry_set_text (MPL_ENTRY (priv->entry), retweet_msg);
  clutter_actor_grab_key_focus (priv->entry);
  g_free (retweet_msg);
}

static ClutterActor *
_bridge_factory_func (MpsViewBridge *bridge,
                      SwItem        *item,
                      gpointer       userdata)
{
  ClutterActor *actor;

  actor = g_object_new (MPS_TYPE_TWEET_CARD,
                        "item", item,
                        NULL);

  g_signal_connect (actor,
                    "reply-clicked",
                    (GCallback)_card_reply_clicked,
                    userdata);
  g_signal_connect (actor,
                    "retweet-clicked",
                    (GCallback)_card_retweet_clicked,
                    userdata);

  return actor;
}


static void
mps_feed_pane_init (MpsFeedPane *self)
{
  MpsFeedPanePrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_text;
  ClutterActor *entry;

  /* Actor creation */
  priv->entry = (ClutterActor *) mpl_entry_new (_("Update"));
  mx_stylable_set_style_class (MX_STYLABLE (priv->entry),
                               "mps-status-entry");
  entry = (ClutterActor *)mpl_entry_get_mx_entry (MPL_ENTRY (priv->entry));
  mx_entry_set_hint_text (MX_ENTRY (entry),
                          _("What's happening?"));
  tmp_text = mx_entry_get_clutter_text (MX_ENTRY (entry));
  g_signal_connect (tmp_text,
                    "activate",
                    (GCallback)_entry_activate_cb,
                    self);

  priv->update_hbox = mx_table_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->update_hbox),
                               "mps-status-update-hbox");

  priv->scroll_view = mx_scroll_view_new ();

  priv->box_layout = mx_box_layout_new ();
  mx_box_layout_set_vertical (MX_BOX_LAYOUT (priv->box_layout), TRUE);

  priv->bridge = mps_view_bridge_new ();
  mps_view_bridge_set_factory_func (priv->bridge,
                                    _bridge_factory_func,
                                    self);
  mps_view_bridge_set_container (priv->bridge,
                                 CLUTTER_CONTAINER (priv->box_layout));

  priv->something_wrong_frame = mx_frame_new ();
  priv->something_wrong_label = mx_label_new (_("Unable to update status. "
                                                "The service may be unavailable "
                                                "or your password could be wrong."));
  mx_stylable_set_style_class (MX_STYLABLE (priv->something_wrong_label),
                               "mps-something-wrong-message");
  mx_stylable_set_style_class (MX_STYLABLE (priv->something_wrong_frame),
                               "mps-something-wrong-frame");

  mx_table_set_row_spacing (MX_TABLE (self), 8);

  priv->geotag_pane = mps_geotag_pane_new ();

  priv->location_hbox = mx_table_new ();
  priv->location_label = mx_label_new ("");
  mx_table_add_actor_with_properties (MX_TABLE (priv->location_hbox),
                                      priv->location_label,
                                      0, 0,
                                      "x-expand", TRUE,
                                      NULL);

  priv->location_button = mx_button_new_with_label (_("Change"));
  mx_table_add_actor_with_properties (MX_TABLE (priv->location_hbox),
                                      priv->location_button,
                                      0, 1,
                                      "x-expand", FALSE,
                                      NULL);

  /* Container population */

  mx_table_add_actor_with_properties (MX_TABLE (priv->update_hbox),
                                      priv->entry,
                                      0, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      NULL);

  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->update_hbox,
                                      0, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      NULL);
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->something_wrong_frame,
                                      0, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      NULL);

  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->location_hbox,
                                      1, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      NULL);

  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->scroll_view,
                                      2, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", TRUE,
                                      "y-fill", TRUE,
                                      NULL);
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->geotag_pane,
                                      2, 0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", TRUE,
                                      "y-fill", TRUE,
                                      NULL);

  clutter_actor_hide (priv->geotag_pane);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->scroll_view),
                               priv->box_layout);

  mx_bin_set_child (MX_BIN (priv->something_wrong_frame),
                    priv->something_wrong_label);

  /* Signals */
  g_signal_connect (priv->entry,
                    "button-clicked",
                    (GCallback)_update_button_clicked_cb,
                    self);

  g_signal_connect (priv->location_button,
                    "clicked",
                    (GCallback)_location_button_clicked_cb,
                    self);

  g_signal_connect (priv->geotag_pane,
                    "location-chosen",
                    (GCallback)_geotag_pane_location_chosen,
                    self);

  clutter_actor_hide (priv->something_wrong_frame);

  _update_location_label (self);
}

ClutterActor *
mps_feed_pane_new (SwClient        *client,
                   SwClientService *service)
{
  return g_object_new (MPS_TYPE_FEED_PANE,
                       "client", client,
                       "service", service,
                       NULL);
}


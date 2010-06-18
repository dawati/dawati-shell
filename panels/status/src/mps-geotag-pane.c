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

#include <champlain/champlain.h>
#include <geoclue/geoclue-master.h>
#include <geoclue/geoclue-geocode.h>
#include <geoclue/geoclue-reverse-geocode.h>
#include "mps-geotag-pane.h"

#include <glib/gi18n.h>

#include <meego-panel/mpl-entry.h>
#include <gconf/gconf-client.h>

G_DEFINE_TYPE (MpsGeotagPane, mps_geotag_pane, MX_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPS_TYPE_GEOTAG_PANE, MpsGeotagPanePrivate))

typedef struct _MpsGeotagPanePrivate MpsGeotagPanePrivate;

enum
{
  LOCATION_CHOSEN,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LATITUDE,
  PROP_LONGITUDE,
  PROP_GEOTAG_ENABLED,
  PROP_GUESS_LOCATION,
  PROP_REVERSE_LOCATION
};

struct _MpsGeotagPanePrivate {
  ClutterActor *current_location_label;
  ClutterActor *map_view;
  GeocluePosition *geo_position;
  GeoclueGeocode *geo_geocode;
  GeoclueReverseGeocode *geo_reverse_geocode;
  ChamplainLayer *markers_layer;
  ClutterActor *your_location_marker;
  ClutterActor *entry;
  ClutterActor *guess_location_button;

  GConfClient *gconf_client;

  guint gconf_geotag_notifyid;
  guint gconf_guess_location_notifyid;

  gboolean position_set;
  gdouble latitude, longitude;
  gboolean geotag_enabled;

  ClutterActor *button_box;
  ClutterActor *use_location_button;
  ClutterActor *dont_use_location_button;

  gchar *reverse_location;
};

static guint signals[LAST_SIGNAL] = { 0, };

#define STATUS_PANEL_GCONF_DIR "/desktop/meego/status"
#define STATUS_PANEL_GEOTAG_KEY "/desktop/meego/status/enable_geotag"
#define STATUS_PANEL_LATITUDE_KEY "/desktop/meego/status/saved_latitude"
#define STATUS_PANEL_LONGITUDE_KEY "/desktop/meego/status/saved_longitude"
#define STATUS_PANEL_GUESS_LOCATION_KEY "/desktop/meego/status/guess_location"

static void
mps_geotag_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_GEOTAG_ENABLED:
      g_value_set_boolean (value, priv->geotag_enabled);
      break;
    case PROP_LATITUDE:
      g_value_set_double (value, priv->latitude);
      break;
    case PROP_LONGITUDE:
      g_value_set_double (value, priv->longitude);
      break;
    case PROP_GUESS_LOCATION:
      g_value_set_boolean (value,
                           mx_button_get_toggled (MX_BUTTON (priv->guess_location_button)));
      break;
    case PROP_REVERSE_LOCATION:
      g_value_set_string (value,
                          priv->reverse_location);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_geotag_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_geotag_pane_dispose (GObject *object)
{
  G_OBJECT_CLASS (mps_geotag_pane_parent_class)->dispose (object);
}

static void
mps_geotag_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (mps_geotag_pane_parent_class)->finalize (object);
}

static void
mps_geotag_pane_class_init (MpsGeotagPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;
  g_type_class_add_private (klass, sizeof (MpsGeotagPanePrivate));

  object_class->get_property = mps_geotag_pane_get_property;
  object_class->set_property = mps_geotag_pane_set_property;
  object_class->dispose = mps_geotag_pane_dispose;
  object_class->finalize = mps_geotag_pane_finalize;

  pspec = g_param_spec_boolean ("geotag-enabled",
                                "Enable geotag",
                                "Geotagging is enabled",
                                FALSE,
                                G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_GEOTAG_ENABLED, pspec);

  pspec = g_param_spec_double ("latitude",
                               "Latitude",
                               "The latitude for the tag",
                               -G_MAXDOUBLE,
                               G_MAXDOUBLE,
                               0.0,
                               G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_LATITUDE, pspec);

  pspec = g_param_spec_double ("longitude",
                               "Longitude",
                               "The longitude for the tag",
                               -G_MAXDOUBLE,
                               G_MAXDOUBLE,
                               0.0,
                               G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_LONGITUDE, pspec);

  pspec = g_param_spec_boolean ("guess-location",
                                "Guess location",
                                "Location was guessed",
                                FALSE,
                                G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_GUESS_LOCATION, pspec);

  pspec = g_param_spec_string ("reverse-location",
                               "Reverse geocoded location",
                               "The human readable location that was the "
                               "result of reverse geocoding the lat and lon",
                               NULL,
                               G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_REVERSE_LOCATION, pspec);

  signals[LOCATION_CHOSEN] = g_signal_new ("location-chosen",
                                           MPS_TYPE_GEOTAG_PANE,
                                           G_SIGNAL_RUN_FIRST,
                                           0,
                                           NULL,
                                           NULL,
                                           g_cclosure_marshal_VOID__VOID,
                                           G_TYPE_NONE,
                                           0);
}

static void
mps_geotag_pane_ensure_marker_visible (MpsGeotagPane *pane)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);

  ChamplainBaseMarker *markers[2] = { NULL, };

  markers[0] = CHAMPLAIN_BASE_MARKER (priv->your_location_marker);
  markers[1] = NULL;

  champlain_view_ensure_markers_visible (CHAMPLAIN_VIEW (priv->map_view),
                                         markers,
                                         FALSE);
}

static void
mps_geotag_pane_update_marker (MpsGeotagPane *pane,
                               gdouble        latitude,
                               gdouble        longitude)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);

  if (!priv->your_location_marker)
  {
    ClutterColor text_color, marker_color;

    clutter_color_from_string (&text_color, "#7dbe0cff");
    clutter_color_from_string (&marker_color, "white");

    priv->your_location_marker = champlain_marker_new_with_text (_("Your location"),
                                                                 "Droid Sans 12",
                                                                 &text_color,
                                                                 &marker_color);
    champlain_layer_add_marker (priv->markers_layer, CHAMPLAIN_BASE_MARKER (priv->your_location_marker));

  }

  champlain_base_marker_set_position (CHAMPLAIN_BASE_MARKER (priv->your_location_marker),
                                      latitude,
                                      longitude);

  g_object_notify (G_OBJECT (pane), "latitude");
  g_object_notify (G_OBJECT (pane), "longitude");
}

static void
_position_get_position_cb (GeocluePosition       *position,
                           GeocluePositionFields  fields,
                           int                    timestamp,
                           double                 latitude,
                           double                 longitude,
                           double                 altitude,
                           GeoclueAccuracy       *accuracy,
                           GError                *error,
                           gpointer               userdata)
{
  MpsGeotagPane *pane = MPS_GEOTAG_PANE (userdata);
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);
  gchar *tmp;

  if (error)
  {
    g_warning (G_STRLOC ": Error getting position: %s",
               error->message);
    return;
  }


  priv->position_set = TRUE;
  priv->latitude = latitude;
  priv->longitude = longitude;
  mps_geotag_pane_update_marker (pane, latitude, longitude);
  mps_geotag_pane_ensure_marker_visible (pane);
}

static void
mps_geotag_pane_guess_location (MpsGeotagPane *pane)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);

  /* Remove existing marker */
  priv->position_set = FALSE;
  if (priv->your_location_marker)
  {
    clutter_actor_destroy (priv->your_location_marker);
    priv->your_location_marker = NULL;
  }

  geoclue_position_get_position_async (priv->geo_position,
                                       _position_get_position_cb,
                                       pane);
}

static void
_reverse_geocode_cb (GeoclueReverseGeocode *rev_geocode,
                     GHashTable            *details,
                     GeoclueAccuracy       *accuracy,
                     GError                *error,
                     gpointer               userdata)
{
  MpsGeotagPane *pane = MPS_GEOTAG_PANE (userdata);
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);

  if (error)
  {
    g_warning (G_STRLOC ": Error reverse geocoding: %s", error->message);
    g_free (priv->reverse_location);
    priv->reverse_location = NULL;
    return;
  }

  g_free (priv->reverse_location);

  priv->reverse_location = g_strdup (g_hash_table_lookup (details,
                                                          GEOCLUE_ADDRESS_KEY_AREA));
  if (!priv->reverse_location)
    priv->reverse_location = g_strdup (g_hash_table_lookup (details,
                                                            GEOCLUE_ADDRESS_KEY_LOCALITY));
  if (!priv->reverse_location)
    priv->reverse_location = g_strdup (g_hash_table_lookup (details,
                                                            GEOCLUE_ADDRESS_KEY_REGION));
  if (!priv->reverse_location)
    priv->reverse_location = g_strdup (g_hash_table_lookup (details,
                                                            GEOCLUE_ADDRESS_KEY_COUNTRY));

  g_object_notify (G_OBJECT (pane), "reverse-location");
}

static void
mps_geotag_pane_reverse_point (MpsGeotagPane *pane)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);
  GeoclueAccuracy *accuracy;

  accuracy = geoclue_accuracy_new (GEOCLUE_ACCURACY_LEVEL_LOCALITY, 0, 0);
  geoclue_reverse_geocode_position_to_address_async (priv->geo_reverse_geocode,
                                                     priv->latitude,
                                                     priv->longitude,
                                                     accuracy,
                                                     _reverse_geocode_cb,
                                                     pane);
  geoclue_accuracy_free (accuracy);

}

static void
_geocode_address_to_position_cb (GeoclueGeocode        *geocode,
                                 GeocluePositionFields  fields,
                                 double                 latitude,
                                 double                 longitude,
                                 double                 altitude,
                                 GeoclueAccuracy       *accuracy,
                                 GError                *error,
                                 gpointer               userdata)
{
  MpsGeotagPane *pane = MPS_GEOTAG_PANE (userdata);
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);

  priv->position_set = TRUE;
  priv->latitude = latitude;
  priv->longitude = longitude;

  mps_geotag_pane_update_marker (pane, latitude, longitude);
  mps_geotag_pane_ensure_marker_visible (pane);
}

static void
_search_for_location (MpsGeotagPane *pane)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);
  GHashTable *details;
  const gchar *str;

  str = mpl_entry_get_text (MPL_ENTRY (priv->entry));

  if (str)
  {
    details = g_hash_table_new (g_str_hash, g_str_equal);
    g_hash_table_insert (details, "locality", (gchar *)str);

    geoclue_geocode_address_to_position_async (priv->geo_geocode,
                                               details,
                                               _geocode_address_to_position_cb,
                                               pane);
    g_hash_table_unref (details);
  }

  mx_button_set_toggled (MX_BUTTON (priv->guess_location_button), FALSE);
}

static void
_location_search_button_clicked_cb (MplEntry      *entry,
                                    MpsGeotagPane *pane)
{
  _search_for_location (pane);
}

static void
_location_search_entry_activated_cb (ClutterText   *text,
                                     MpsGeotagPane *pane)
{
  _search_for_location (pane);
}

static gboolean
_map_view_button_release_event_cb (ClutterActor  *actor,
                                   ClutterEvent  *event,
                                   MpsGeotagPane *pane)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);
  ClutterButtonEvent *bevent = (ClutterButtonEvent *)event;
  gdouble latitude, longitude;

  /* We only care about single clicks! */
  if (bevent->button != 1 || bevent->click_count != 1)
    return FALSE;

  if (champlain_view_get_coords_from_event (CHAMPLAIN_VIEW (priv->map_view),
                                            event,
                                            &latitude,
                                            &longitude))
  {
    priv->latitude = latitude;
    priv->longitude = longitude;
    priv->position_set = TRUE;
    mps_geotag_pane_update_marker (pane, latitude, longitude);
    mx_button_set_toggled (MX_BUTTON (priv->guess_location_button), FALSE);
    return TRUE;
  }
  return FALSE;
}

static gboolean
_guess_location_checked_notify_cb (MxButton      *button,
                                   GParamSpec    *pspec,
                                   MpsGeotagPane *pane)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);

  if (mx_button_get_toggled (button))
  {
    mps_geotag_pane_guess_location (pane);
  } else {
    /* Leave marker as it was */
  }
}

static void
_gconf_enable_geotag_notify_cb (GConfClient *client,
                                guint        cnxn_id,
                                GConfEntry  *entry,
                                gpointer     userdata)
{
  MpsGeotagPane *pane = MPS_GEOTAG_PANE (userdata);
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);

  if (gconf_entry_get_value (entry))
  {
    GConfValue *value = gconf_entry_get_value (entry);

    priv->geotag_enabled = gconf_value_get_bool (value);
  } else {
    priv->geotag_enabled = FALSE;
  }
}

static void
_gconf_guess_location_notify_cb (GConfClient *client,
                                 guint        cnxn_id,
                                 GConfEntry  *entry,
                                 gpointer     userdata)
{
  MpsGeotagPane *pane = MPS_GEOTAG_PANE (userdata);
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);
  gboolean guess_location = TRUE;

  if (!gconf_entry_get_value (entry))
  {
    guess_location = TRUE;
  } else {
    GConfValue *value = gconf_entry_get_value (entry);

    if (gconf_value_get_bool (value))
      guess_location = TRUE;
    else
      guess_location = FALSE;
  }

  g_signal_handlers_block_by_func (priv->guess_location_button,
                                   _guess_location_checked_notify_cb,
                                   userdata);
  mx_button_set_toggled (MX_BUTTON (priv->guess_location_button),
                         guess_location);
  g_signal_handlers_unblock_by_func (priv->guess_location_button,
                                     _guess_location_checked_notify_cb,
                                     userdata);

  if (guess_location)
    mps_geotag_pane_guess_location (pane);
}

static void
_use_location_button_clicked_cb (MxButton      *button,
                                 MpsGeotagPane *pane)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);
  gboolean guess_location;

  guess_location = mx_button_get_toggled (MX_BUTTON (priv->guess_location_button));

  gconf_client_set_bool (priv->gconf_client,
                         STATUS_PANEL_GUESS_LOCATION_KEY,
                         guess_location,
                         NULL);

  if (priv->position_set)
  {
    gconf_client_set_float (priv->gconf_client,
                            STATUS_PANEL_LATITUDE_KEY,
                            priv->latitude,
                            NULL);
    gconf_client_set_float (priv->gconf_client,
                            STATUS_PANEL_LONGITUDE_KEY,
                            priv->longitude,
                            NULL);
  }

  gconf_client_set_bool (priv->gconf_client,
                         STATUS_PANEL_GEOTAG_KEY,
                         priv->position_set,
                         NULL);

  priv->geotag_enabled = TRUE;
  mps_geotag_pane_reverse_point (pane);
  g_signal_emit (pane, signals [LOCATION_CHOSEN], 0);
}

static void
_dont_use_location_button_clicked_cb (MxButton      *button,
                                      MpsGeotagPane *pane)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (pane);

  gconf_client_unset (priv->gconf_client, STATUS_PANEL_LATITUDE_KEY, NULL);
  gconf_client_unset (priv->gconf_client, STATUS_PANEL_LONGITUDE_KEY, NULL);

  gconf_client_set_bool (priv->gconf_client, STATUS_PANEL_GEOTAG_KEY, FALSE, NULL);

  priv->geotag_enabled = FALSE;
  g_signal_emit (pane, signals [LOCATION_CHOSEN], 0);
}

static void
mps_geotag_pane_init (MpsGeotagPane *self)
{
  MpsGeotagPanePrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;
  ClutterActor *entry;

  priv->map_view = champlain_view_new ();
  priv->markers_layer = champlain_layer_new ();
  champlain_view_add_layer (CHAMPLAIN_VIEW (priv->map_view), priv->markers_layer);
  clutter_actor_set_reactive (priv->map_view, TRUE);
  champlain_view_set_zoom_level (CHAMPLAIN_VIEW (priv->map_view), 7);

  priv->entry = (ClutterActor *)mpl_entry_new (_("Search"));
  mx_stylable_set_style_class (MX_STYLABLE (priv->entry),
                               "mps-geo-search-entry");
  entry = (ClutterActor *)mpl_entry_get_mx_entry (MPL_ENTRY (priv->entry));
  mx_entry_set_hint_text (MX_ENTRY (entry),
                          _("Where are you?"));

  priv->guess_location_button = mx_button_new_with_label (_("Find me"));
  mx_button_set_is_toggle (MX_BUTTON (priv->guess_location_button), TRUE);

  priv->current_location_label = mx_label_new ();

  priv->button_box = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->button_box), 6);
  priv->use_location_button = mx_button_new_with_label (_("Use location"));
  priv->dont_use_location_button = mx_button_new_with_label (_("Don't use location"));

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                               priv->dont_use_location_button);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                               priv->use_location_button);

  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->entry,
                                      1, 0,
                                      "x-expand",TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      NULL);
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->guess_location_button,
                                      1, 1,
                                      "x-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      NULL);

  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->map_view,
                                      2, 0,
                                      "x-expand",TRUE,
                                      "x-fill", TRUE,
                                      "y-expand", TRUE,
                                      "y-fill", TRUE,
                                      "column-span", 2,
                                      NULL);

  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->button_box,
                                      3, 0,
                                      "x-expand", TRUE,
                                      "x-align", MX_ALIGN_END,
                                      "x-fill", FALSE,
                                      "column-span", 2,
                                      "y-expand", FALSE,
                                      NULL);

  mx_table_set_column_spacing (MX_TABLE (self), 6);
  mx_table_set_row_spacing (MX_TABLE (self), 6);

  priv->geo_position = geoclue_position_new ("org.freedesktop.Geoclue.Providers.Hostip",
                                             "/org/freedesktop/Geoclue/Providers/Hostip");

  priv->geo_geocode = geoclue_geocode_new ("org.freedesktop.Geoclue.Providers.Yahoo",
                                           "/org/freedesktop/Geoclue/Providers/Yahoo");

  priv->geo_reverse_geocode = geoclue_reverse_geocode_new ("org.freedesktop.Geoclue.Providers.Geonames",
                                                           "/org/freedesktop/Geoclue/Providers/Geonames");

  g_signal_connect (priv->entry,
                    "button-clicked",
                    (GCallback)_location_search_button_clicked_cb,
                    self);

  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (entry)),
                    "activate",
                    (GCallback)_location_search_entry_activated_cb,
                    self);

  g_signal_connect (priv->map_view,
                    "button-release-event",
                    (GCallback)_map_view_button_release_event_cb,
                    self);

  g_signal_connect (priv->guess_location_button,
                    "notify::checked",
                    (GCallback)_guess_location_checked_notify_cb,
                    self);

  g_signal_connect (priv->use_location_button,
                    "clicked",
                    (GCallback)_use_location_button_clicked_cb,
                    self);

  g_signal_connect (priv->dont_use_location_button,
                    "clicked",
                    (GCallback)_dont_use_location_button_clicked_cb,
                    self);

  priv->gconf_client = gconf_client_get_default ();
  gconf_client_add_dir (priv->gconf_client,
                        STATUS_PANEL_GCONF_DIR,
                        GCONF_CLIENT_PRELOAD_ONELEVEL,
                        &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error add directory to gconf: %s",
               error->message);
    g_clear_error (&error);
  }

  priv->gconf_geotag_notifyid = gconf_client_notify_add (priv->gconf_client,
                                                         STATUS_PANEL_GEOTAG_KEY,
                                                         _gconf_enable_geotag_notify_cb,
                                                         self,
                                                         NULL,
                                                         &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error setting up gconf notification: %s",
               error->message);
    g_clear_error (&error);
  }

  priv->gconf_guess_location_notifyid = gconf_client_notify_add (priv->gconf_client,
                                                                 STATUS_PANEL_GUESS_LOCATION_KEY,
                                                                 _gconf_guess_location_notify_cb,
                                                                 self,
                                                                 NULL,
                                                                 &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error setting up gconf notification: %s",
               error->message);
    g_clear_error (&error);
  }

  gconf_client_notify (priv->gconf_client, STATUS_PANEL_GUESS_LOCATION_KEY);
  gconf_client_notify (priv->gconf_client, STATUS_PANEL_GEOTAG_KEY);

  {
    GConfValue *latitude_value = gconf_client_get_without_default (priv->gconf_client,
                                                                   STATUS_PANEL_LATITUDE_KEY,
                                                                   NULL);
    GConfValue *longitude_value = gconf_client_get_without_default (priv->gconf_client,
                                                                    STATUS_PANEL_LONGITUDE_KEY,
                                                                    NULL);

    if (latitude_value && longitude_value)
    {
      priv->latitude = gconf_value_get_float (latitude_value);
      priv->longitude = gconf_value_get_float (longitude_value);
      priv->position_set = TRUE;

      mps_geotag_pane_update_marker (self, priv->latitude, priv->longitude);
      mps_geotag_pane_ensure_marker_visible (self);
      mps_geotag_pane_reverse_point (self);
    } else {
      gconf_client_set_bool (priv->gconf_client,
                             STATUS_PANEL_GUESS_LOCATION_KEY,
                             TRUE,
                             NULL);
    }

    if (latitude_value)
      gconf_value_free (latitude_value);

    if (longitude_value)
      gconf_value_free (longitude_value);
  }
}

ClutterActor *
mps_geotag_pane_new (void)
{
  return g_object_new (MPS_TYPE_GEOTAG_PANE, NULL);
}


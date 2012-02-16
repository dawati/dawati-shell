
/*
 * Copyright © 2010, 2012 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
 *          Damien Lespiau <damien.lespiau@intel.com>
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

#include <stdbool.h>

#include <glib/gi18n.h>

#include "carrick/carrick-connman-manager.h"

#include "mpd-battery-tile.h"
#include "mpd-brightness-tile.h"
#include "mpd-computer-tile.h"
#include "mpd-display-device.h"
#include "mpd-shell-defines.h"
#include "mpd-volume-tile.h"
#include "config.h"

G_DEFINE_TYPE (MpdComputerTile, mpd_computer_tile, MX_TYPE_TABLE)

#define COMPUTER_TILE_PRIVATE(o)                              \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o),                    \
                                      MPD_TYPE_COMPUTER_TILE, \
                                      MpdComputerTilePrivate))

typedef enum
{
  RADIO_WIFI,
  RADIO_WIMAX,
  RADIO_BLUETOOTH,
  RADIO_3G,

  N_RADIO_TECHS
} RadioTechs;

#define OFFLINE_MODE  N_RADIO_TECHS

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

typedef struct
{
  ClutterActor *label, *frame, *toggle;
} RadioRow;

typedef struct
{
  MpdComputerTile *tile;
  int row;
} ToggledData;

struct _MpdComputerTilePrivate
{
  CarrickConnmanManager *cm;

  RadioRow rows[N_RADIO_TECHS + 1]; /* techs + Offline mode */

  /* Those structures are used by the notify::active handlers */
  ToggledData toggled_data[N_RADIO_TECHS + 1];
};

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static const gchar *
radio_tech_to_connman_tech (RadioTechs tech)
{
  static const char *connman_techs[N_RADIO_TECHS] =
    { "wifi", "wimax", "bluetooth", "cellular" };

  g_assert (tech >= 0 && tech < N_RADIO_TECHS);

  return connman_techs[tech];
}

static void
on_switch_toggled (MxToggle    *toggle,
                   GParamSpec  *property,
                   ToggledData *data)
{
  MpdComputerTilePrivate *priv = data->tile->priv;
  gboolean active;

  active = mx_toggle_get_active (toggle);
  carrick_connman_manager_set_technology_state (priv->cm,
                                                radio_tech_to_connman_tech (data->row),
                                                active);
}

static void
show_tech (MpdComputerTile *tile,
           RadioTechs       tech,
           gboolean         available)
{
  MpdComputerTilePrivate *priv = tile->priv;

  if (available)
    {
      clutter_actor_show (priv->rows[tech].label);
      clutter_actor_show (priv->rows[tech].frame);
    }
  else
    {
      clutter_actor_hide (priv->rows[tech].label);
      clutter_actor_hide (priv->rows[tech].frame);
    }
}

static void
show_airplane_mode (MpdComputerTile *tile,
                    gboolean         available)
{
  show_tech (tile, OFFLINE_MODE, available);
}

static void
_available_techs_changed (CarrickConnmanManager *cm,
                          GParamSpec            *pspec,
                          MpdComputerTile       *tile)
{
  gboolean available_techs[N_RADIO_TECHS];
  char **techs, **iter;
  int show_airplane = 0, i;

  memset (available_techs, 0, N_RADIO_TECHS * sizeof (gboolean));

  g_object_get (cm, "available-technologies", &techs, NULL);
  iter = techs;

  while (*iter)
    {
      for (i = 0; i < N_RADIO_TECHS; i++)
        if (g_strcmp0 (*iter, radio_tech_to_connman_tech (i)) == 0)
          available_techs[i] = TRUE;
      iter++;
    }

  for (i = 0; i < N_RADIO_TECHS; i++)
    {
      if (available_techs[i])
        {
          show_airplane++;
          show_tech (tile, i, TRUE);
        }
      else
        {
          show_tech (tile, i, FALSE);
        }
    }

  show_airplane_mode (tile, show_airplane);

  g_strfreev (techs);
}

static void
_enabled_techs_changed (CarrickConnmanManager *cm,
                        GParamSpec            *pspec,
                        MpdComputerTile       *tile)
{
  MpdComputerTilePrivate *priv = tile->priv;
  gboolean enabled_techs[N_RADIO_TECHS];
  char **techs, **iter;
  int i;

  memset (enabled_techs, 0, N_RADIO_TECHS * sizeof (gboolean));

  g_object_get (cm, "enabled-technologies", &techs, NULL);
  iter = techs;

  while (*iter) {
      for (i = 0; i < N_RADIO_TECHS; i++)
        if (g_strcmp0 (*iter, radio_tech_to_connman_tech (i)) == 0)
          enabled_techs[i] = TRUE;
    iter++;
  }

  for (i = 0; i < N_RADIO_TECHS; i++)
    {
      g_signal_handlers_block_by_func (priv->rows[i].toggle,
                                       on_switch_toggled,
                                       &priv->toggled_data[i]);
      mx_toggle_set_active (MX_TOGGLE (priv->rows[i].toggle), enabled_techs[i]);
      g_signal_handlers_unblock_by_func (priv->rows[i].toggle,
                                         on_switch_toggled,
                                         &priv->toggled_data[i]);
    }

  g_strfreev (techs);
}


/*
 * GObject implementation
 */

static void
_dispose (GObject *object)
{
  MpdComputerTilePrivate *priv = MPD_COMPUTER_TILE (object)->priv;

  if (priv->cm)
    {
      g_object_unref (priv->cm);
      priv->cm = NULL;
    }

  G_OBJECT_CLASS (mpd_computer_tile_parent_class)->dispose (object);
}

static void
mpd_computer_tile_class_init (MpdComputerTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdComputerTilePrivate));

  object_class->dispose = _dispose;

  /* Signals */

  _signals[REQUEST_HIDE] = g_signal_new ("request-hide",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);
}

static void
create_network_row (MpdComputerTile *tile,
                    const gchar     *label_text,
                    gint             row)
{
  MpdComputerTilePrivate *priv = tile->priv;
  ClutterActor *label, *toggle, *frame;

  label = mx_label_new_with_text (label_text);
  mx_table_add_actor_with_properties (MX_TABLE (tile), label,
                                      row, 0,
                                      "x-expand", FALSE,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-fill", FALSE,
                                      NULL);

  frame = mx_frame_new ();
  toggle = mx_toggle_new ();

  mx_bin_set_child (MX_BIN (frame), toggle);
  mx_table_add_actor_with_properties (MX_TABLE (tile), frame,
                                      row, 1,
                                      "x-expand", FALSE,
                                      "x-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);

  priv->toggled_data[row].tile = tile;
  priv->toggled_data[row].row = row;
  g_signal_connect (toggle,
                    "notify::active",
                    G_CALLBACK (on_switch_toggled),
                    &priv->toggled_data[row]);

  priv->rows[row].label = label;
  priv->rows[row].frame = frame;
  priv->rows[row].toggle = toggle;
}

static void
mpd_computer_tile_init (MpdComputerTile *self)
{
  MpdComputerTilePrivate *priv;
  ClutterActor      *tile, *label;
  MpdDisplayDevice  *display;
  bool               show_brightness_tile;

  self->priv = priv = COMPUTER_TILE_PRIVATE (self);

  priv->cm = carrick_connman_manager_new ();
  g_signal_connect (priv->cm, "notify::available-technologies",
                    G_CALLBACK (_available_techs_changed), self);
  g_signal_connect (priv->cm, "notify::enabled-technologies",
                    G_CALLBACK (_enabled_techs_changed), self);


  /* Let's reserve some room (rows) for the different "radios", we need:
   * WiFi (0)
   * Wimax (1)
   * Bluetooth (2)
   * 3G (3)
   * Air plane mode (4) */
#define START_ROW 5

  create_network_row (self, _("Wifi"), RADIO_WIFI);
  create_network_row (self, _("Wimax"), RADIO_WIMAX);
  create_network_row (self, _("Bluetooth"), RADIO_BLUETOOTH);
  create_network_row (self, _("3G"), RADIO_3G);
#if 0
  create_network_row (self, _("Offline mode"), OFFLINE_MODE);
#endif

  show_tech (self, RADIO_WIFI, FALSE);
  show_tech (self, RADIO_WIMAX, FALSE);
  show_tech (self, RADIO_BLUETOOTH, FALSE);
  show_tech (self, RADIO_3G, FALSE);

  /* Volume */
  /* Note to translators, volume here is sound volume */
  label = mx_label_new_with_text (_("Volume"));
  mx_table_add_actor_with_properties (MX_TABLE (self), label,
                                      START_ROW, 0,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-fill", FALSE,
                                      NULL);

  tile = mpd_volume_tile_new ();
  mx_table_add_actor_with_properties (MX_TABLE (self), tile,
                                      START_ROW, 1,
                                      "y-expand", FALSE,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-fill", FALSE,
                                      "x-expand", TRUE,
                                      NULL);

  /* Brightness */
  display = mpd_display_device_new ();
  show_brightness_tile = mpd_display_device_is_enabled (display);
  if (show_brightness_tile)
    {
      label = mx_label_new_with_text (_("Brightness"));
      mx_table_add_actor_with_properties (MX_TABLE (self), label,
                                          START_ROW + 1, 0,
                                          "y-align", MX_ALIGN_MIDDLE,
                                          "y-fill", FALSE,
                                          NULL);

      tile = mpd_brightness_tile_new ();
      mx_table_add_actor_with_properties (MX_TABLE (self), tile,
                                          START_ROW + 1, 1,
                                          "x-expand", TRUE,
                                          NULL);
    }

#if 0
  tile = mpd_battery_tile_new ();
  mx_table_add_actor (MX_TABLE (self), tile, 2, 0);
#endif

  /* FIXME: Makes crash when unref'd.
   * GpmBrightnessXRandR doesn't remove filter from root window in ::finalize()
   * but doesn't seem to be it.
   * g_object_unref (display); */
}

ClutterActor *
mpd_computer_tile_new (void)
{
  return g_object_new (MPD_TYPE_COMPUTER_TILE, NULL);
}



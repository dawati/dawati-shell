
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
 * based on dalston-volume-applet.c by Rob Bradford <rob@linux.intel.com>
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

#include <glib/gi18n.h>
#include <gvc/gvc-mixer-control.h>
#include "mpd-volume-tile.h"

#define MIXER_CONTROL_NAME "Moblin Panel Devices"

GvcMixerStream *
mpd_volume_tile_get_default_sink (MpdVolumeTile   *self);

static void
mpd_volume_tile_set_default_sink (MpdVolumeTile   *self,
                                  GvcMixerStream  *stream);

static void
update_mute_toggle (MpdVolumeTile *self);

static void
update_volume_slider (MpdVolumeTile *self);

static void
update_stream_mute (MpdVolumeTile *self);

static void
update_stream_volume (MpdVolumeTile *self);

G_DEFINE_TYPE (MpdVolumeTile, mpd_volume_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_VOLUME_TILE, MpdVolumeTilePrivate))

typedef struct
{
  /* Managed by clutter */
  ClutterActor    *volume_slider;
  ClutterActor    *mute_toggle;

  /* Data */
  GvcMixerControl *control;
  GvcMixerStream  *default_sink;
  guint            timeout;
} MpdVolumeTilePrivate;

static gboolean
_reopen_pa_timeout_cb (MpdVolumeTile *self)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (self);

  priv->timeout = 0;
  gvc_mixer_control_open (priv->control);
  return FALSE;
}

static void
_mute_toggle_notify_cb (MxToggle      *toggle,
                        GParamSpec    *pspec,
                        MpdVolumeTile *self)
{
  update_stream_mute (self);
}

static void
_volume_slider_progress_notify_cb (MxSlider      *slider,
                                   GParamSpec    *pspec,
                                   MpdVolumeTile *self)
{
  update_stream_volume (self);
}

static void
_stream_is_muted_notify_cb (GObject       *object,
                            GParamSpec    *pspec,
                            MpdVolumeTile *self)
{
  update_mute_toggle (self);
}

static void
_stream_volume_notify_cb (GObject       *object,
                          GParamSpec    *pspec,
                          MpdVolumeTile *self)
{
  update_volume_slider (self);
}

static void
_mixer_control_default_sink_changed_cb (GvcMixerControl *control,
                                        guint            id,
                                        MpdVolumeTile   *self)
{
  mpd_volume_tile_set_default_sink (self,
                                    gvc_mixer_control_get_default_sink (control));
}

static void
_mixer_control_ready_cb (GvcMixerControl  *control,
                         MpdVolumeTile    *self)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (self);

  g_debug (G_STRLOC ": Mixer ready!");

  mpd_volume_tile_set_default_sink (self,
                                    gvc_mixer_control_get_default_sink (priv->control));
}

static void
_mixer_control_connection_failed_cb (GvcMixerControl  *control,
                                     MpdVolumeTile    *self)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (self);

  g_signal_handlers_disconnect_by_func (priv->control,
                                        _mixer_control_default_sink_changed_cb,
                                        self);
  g_signal_handlers_disconnect_by_func (priv->control,
                                        _mixer_control_ready_cb,
                                        self);
  g_signal_handlers_disconnect_by_func (priv->control,
                                        _mixer_control_connection_failed_cb,
                                        self);
  g_object_unref (priv->control);
  priv->control = gvc_mixer_control_new (MIXER_CONTROL_NAME);
  g_signal_connect (priv->control, "default-sink-changed",
                    G_CALLBACK (_mixer_control_default_sink_changed_cb), self);
  g_signal_connect (priv->control, "ready",
                    G_CALLBACK (_mixer_control_ready_cb), self);
  g_signal_connect (priv->control, "connection-failed",
                    G_CALLBACK (_mixer_control_connection_failed_cb), self);

  if (priv->timeout)
  {
      g_source_remove (priv->timeout);
      priv->timeout = 0;
  }

  priv->timeout = g_timeout_add_seconds (10,
                                        (GSourceFunc) _reopen_pa_timeout_cb,
                                         self);
}

static void
_dispose (GObject *object)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (object);

  if (priv->timeout)
  {
    g_source_remove (priv->timeout);
    priv->timeout = 0;
  }

  if (priv->control)
  {
    g_object_unref (priv->control);
    priv->control = NULL;
  }

  G_OBJECT_CLASS (mpd_volume_tile_parent_class)->dispose (object);
}

static void
mpd_volume_tile_class_init (MpdVolumeTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdVolumeTilePrivate));

  object_class->dispose = _dispose;
}

static void
mpd_volume_tile_init (MpdVolumeTile *self)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor  *mute_box;
  ClutterActor  *mute_label;

  mx_box_layout_set_vertical (MX_BOX_LAYOUT (self), TRUE);

  priv->volume_slider = mx_slider_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->volume_slider);
  g_signal_connect (priv->volume_slider, "notify::progress",
                    G_CALLBACK (_volume_slider_progress_notify_cb), self);

  mute_box = mx_box_layout_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (self), mute_box);
  clutter_container_child_set (CLUTTER_CONTAINER (self), mute_box,
                               "expand", FALSE,
                               "x-align", MX_ALIGN_END,
                               "x-fill", FALSE,
                               NULL);

  mute_label = mx_label_new (_("Mute"));
  clutter_container_add_actor (CLUTTER_CONTAINER (mute_box), mute_label);
  clutter_container_child_set (CLUTTER_CONTAINER (mute_box), mute_label,
                               "expand", FALSE,
                               "y-align", MX_ALIGN_MIDDLE,
                               "y-fill", FALSE,
                               NULL);

  priv->mute_toggle = mx_toggle_new ();
  g_signal_connect (priv->mute_toggle, "notify::active",
                    G_CALLBACK (_mute_toggle_notify_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (mute_box), priv->mute_toggle);

  priv->control = gvc_mixer_control_new (MIXER_CONTROL_NAME);
  g_signal_connect (priv->control, "default-sink-changed",
                    G_CALLBACK (_mixer_control_default_sink_changed_cb), self);
  g_signal_connect (priv->control, "ready",
                    G_CALLBACK (_mixer_control_ready_cb), self);
  g_signal_connect (priv->control, "connection-failed",
                    G_CALLBACK (_mixer_control_connection_failed_cb), self);
  gvc_mixer_control_open (priv->control);
}

ClutterActor *
mpd_volume_tile_new (void)
{
  return g_object_new (MPD_TYPE_VOLUME_TILE, NULL);
}

GvcMixerStream *
mpd_volume_tile_get_default_sink (MpdVolumeTile *self)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_VOLUME_TILE (self), NULL);

  return priv->default_sink;
}

static void
mpd_volume_tile_set_default_sink (MpdVolumeTile   *self,
                                  GvcMixerStream  *sink)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (self);

  if (priv->default_sink)
  {
    g_signal_handlers_disconnect_by_func (priv->default_sink,
                                          _stream_is_muted_notify_cb,
                                          self);
    g_signal_handlers_disconnect_by_func (priv->default_sink,
                                          _stream_volume_notify_cb,
                                          self);
    g_object_unref (priv->default_sink);
    priv->default_sink = NULL;
  }

  if (sink)
  {
    priv->default_sink = g_object_ref (sink);

    g_signal_connect (priv->default_sink, "notify::is-muted",
                      G_CALLBACK (_stream_is_muted_notify_cb), self);
    g_signal_connect (priv->default_sink, "notify::volume",
                      G_CALLBACK (_stream_volume_notify_cb), self);
    update_mute_toggle (self);
    update_volume_slider (self);
  }
}

static void
update_mute_toggle (MpdVolumeTile *self)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (self);
  gboolean is_muted;

  g_signal_handlers_disconnect_by_func (priv->mute_toggle,
                                        _mute_toggle_notify_cb,
                                        self);

  is_muted = gvc_mixer_stream_get_is_muted (priv->default_sink);
  mx_toggle_set_active (MX_TOGGLE (priv->mute_toggle), is_muted);
  // TODO mx_widget_set_sensitive?

  g_signal_connect (priv->mute_toggle, "notify::active",
                    G_CALLBACK (_mute_toggle_notify_cb), self);
}

static void
update_volume_slider (MpdVolumeTile *self)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (self);
  gdouble volume;
  gdouble progress;

  g_signal_handlers_disconnect_by_func (priv->volume_slider,
                                        _volume_slider_progress_notify_cb,
                                        self);

  volume = gvc_mixer_stream_get_volume (priv->default_sink);
  progress = volume / PA_VOLUME_NORM;
  mx_slider_set_progress (MX_SLIDER (priv->volume_slider), progress);

  g_signal_connect (priv->volume_slider, "notify::progress",
                    G_CALLBACK (_volume_slider_progress_notify_cb), self);
}

static void
update_stream_mute (MpdVolumeTile *self)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (self);
  gboolean is_muted;

  g_signal_handlers_disconnect_by_func (priv->default_sink,
                                        _stream_is_muted_notify_cb,
                                        self);

  is_muted = mx_toggle_get_active (MX_TOGGLE (priv->mute_toggle));
  gvc_mixer_stream_change_is_muted (priv->default_sink, is_muted);

  g_signal_connect (priv->default_sink, "notify::is-muted",
                    G_CALLBACK (_stream_is_muted_notify_cb), self);

}

static void
update_stream_volume (MpdVolumeTile *self)
{
  MpdVolumeTilePrivate *priv = GET_PRIVATE (self);
  gdouble progress;
  pa_volume_t volume;

  g_signal_handlers_disconnect_by_func (priv->default_sink,
                                        _stream_volume_notify_cb,
                                        self);

  progress = mx_slider_get_progress (MX_SLIDER (priv->volume_slider));
  volume = progress * PA_VOLUME_NORM;
  if (gvc_mixer_stream_set_volume (priv->default_sink, volume))
    gvc_mixer_stream_push_volume (priv->default_sink);
  else
    g_warning ("gvc_mixer_stream_change_volume() failed");

  g_signal_connect (priv->default_sink, "notify::volume",
                    G_CALLBACK (_stream_volume_notify_cb), self);
}

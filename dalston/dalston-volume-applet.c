/*
 * Dalston - power and volume applets for Moblin
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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
 *
 */


#include "dalston-volume-applet.h"

#include <libgvc/gvc-mixer-control.h>
#include <gtk/gtk.h>
#include <nbtk/nbtk-gtk.h>

#include "dalston-volume-pane.h"

G_DEFINE_TYPE (DalstonVolumeApplet, dalston_volume_applet, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_VOLUME_APPLET, DalstonVolumeAppletPrivate))

typedef struct _DalstonVolumeAppletPrivate DalstonVolumeAppletPrivate;

struct _DalstonVolumeAppletPrivate {
  MplPanelClient *panel_client;
  GvcMixerControl *control;
  GvcMixerStream *sink;
  GtkWidget *pane;
  guint timeout;
};

enum
{
  PROP_0,
  PROP_PANEL_CLIENT
};

static void
dalston_volume_applet_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_PANEL_CLIENT:
      g_value_set_object (value, priv->panel_client);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_volume_applet_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_PANEL_CLIENT:
      priv->panel_client = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_volume_applet_dispose (GObject *object)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (object);

  if (priv->timeout)
  {
      g_source_remove (priv->timeout);
      priv->timeout = 0;
  }

  if (priv->panel_client)
  {
    g_object_unref (priv->panel_client);
    priv->panel_client = NULL;
  }

  if (priv->control)
  {
    g_object_unref (priv->control);
    priv->control = NULL;
  }

  if (priv->sink)
  {
    g_object_unref (priv->sink);
    priv->sink = NULL;
  }

  G_OBJECT_CLASS (dalston_volume_applet_parent_class)->dispose (object);
}

static void
dalston_volume_applet_finalize (GObject *object)
{
  G_OBJECT_CLASS (dalston_volume_applet_parent_class)->finalize (object);
}

static void
dalston_volume_applet_class_init (DalstonVolumeAppletClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (DalstonVolumeAppletPrivate));

  object_class->get_property = dalston_volume_applet_get_property;
  object_class->set_property = dalston_volume_applet_set_property;
  object_class->dispose = dalston_volume_applet_dispose;
  object_class->finalize = dalston_volume_applet_finalize;

  pspec = g_param_spec_object ("panel-client",
                               "Panel client",
                               "The panel client",
                               MPL_TYPE_PANEL_CLIENT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_PANEL_CLIENT, pspec);
}

static void
dalston_volume_applet_update_icon (DalstonVolumeApplet *self)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (self);
  guint volume;

  if (gvc_mixer_stream_get_is_muted (priv->sink))
  {
    // FIXME robsta where is the "mute" icon?
    mpl_panel_client_request_button_style (priv->panel_client, "state-0");
  } else {
    volume = 100 *
      ((float)gvc_mixer_stream_get_volume (priv->sink) / PA_VOLUME_NORM);

    if (volume == 0)
    {
      mpl_panel_client_request_button_style (priv->panel_client, "state-0");
    } else if (volume < 45) {
      mpl_panel_client_request_button_style (priv->panel_client, "state-1");
    } else if (volume < 75) {
      mpl_panel_client_request_button_style (priv->panel_client, "state-2");
    } else {
      mpl_panel_client_request_button_style (priv->panel_client, "state-3");
    }
  }
}

static void
_mixer_control_ready_cb (GvcMixerControl *control,
                         gpointer         userdata)
{
  g_debug (G_STRLOC ": Mixer ready!");
}

static void
_stream_is_muted_notify_cb (GObject    *object,
                            GParamSpec *pspec,
                            gpointer    self)
{
  dalston_volume_applet_update_icon ((DalstonVolumeApplet *)self);
}

static void
_stream_volume_notify_cb (GObject    *object,
                          GParamSpec *pspec,
                          gpointer    self)
{
  dalston_volume_applet_update_icon ((DalstonVolumeApplet *)self);
}

static void
dalston_volume_applet_update_default_sink (DalstonVolumeApplet  *self,
                                           GvcMixerControl      *control)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (self);

  if (priv->sink)
  {
    g_signal_handlers_disconnect_by_func (priv->sink,
                                          _stream_is_muted_notify_cb,
                                          self);
    g_signal_handlers_disconnect_by_func (priv->sink,
                                          _stream_volume_notify_cb,
                                          self);
    g_object_unref (priv->sink);
    priv->sink = NULL;
  }

  priv->sink = gvc_mixer_control_get_default_sink (control);
  if (priv->sink)
  {
    g_object_ref (priv->sink);

    g_signal_connect (priv->sink,
                      "notify::is-muted",
                      (GCallback)_stream_is_muted_notify_cb,
                      self);
    g_signal_connect (priv->sink,
                      "notify::volume",
                      (GCallback)_stream_volume_notify_cb,
                      self);
    dalston_volume_applet_update_icon (self);
    g_object_set (priv->pane,
                  "sink", priv->sink,
                  NULL);
  } else {
    g_object_set (priv->pane,
                  "sink", NULL,
                  NULL);
  }
}

static gboolean
_reopen_pa_timeout_cb (gpointer data)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (data);

  priv->timeout = 0;
  gvc_mixer_control_open (priv->control);
  return FALSE;
}

static void
_mixer_control_connection_failed_cb (GvcMixerControl *control,
				     gpointer         userdata)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (userdata);

  g_signal_handlers_disconnect_by_func (priv->control,
					_mixer_control_default_sink_changed_cb,
					userdata);
  g_signal_handlers_disconnect_by_func (priv->control,
					_mixer_control_ready_cb,
					userdata);
  g_signal_handlers_disconnect_by_func (priv->control,
					_mixer_control_connection_failed_cb,
					userdata);

  priv->control = gvc_mixer_control_new ();
  g_signal_connect (priv->control,
                    "default-sink-changed",
                    (GCallback)_mixer_control_default_sink_changed_cb,
                    userdata);
  g_signal_connect (priv->control,
                    "ready",
                    (GCallback)_mixer_control_ready_cb,
                    userdata);
  g_signal_connect (priv->control,
                    "connection-failed",
                    (GCallback)_mixer_control_connection_failed_cb,
                    userdata);

  if (priv->timeout)
  {
      g_source_remove (priv->timeout);
      priv->timeout = 0;
  }

  priv->timeout = g_timeout_add (10000,
				 _reopen_pa_timeout_cb,
				 userdata);
}

static void
_mixer_control_default_sink_changed_cb (GvcMixerControl *control,
                                        guint            id,
                                        gpointer         self)
{
  dalston_volume_applet_update_default_sink (self, control);
}

static void
dalston_volume_applet_init (DalstonVolumeApplet *self)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (self);

  priv->pane = dalston_volume_pane_new ();

  priv->control = gvc_mixer_control_new ();
  dalston_volume_applet_update_default_sink (self, priv->control);
  g_signal_connect (priv->control,
                    "default-sink-changed",
                    (GCallback)_mixer_control_default_sink_changed_cb,
                    self);
  g_signal_connect (priv->control,
                    "ready",
                    (GCallback)_mixer_control_ready_cb,
                    self);
  g_signal_connect (priv->control,
                    "connection-failed",
                    (GCallback)_mixer_control_connection_failed_cb,
                    self);
  gvc_mixer_control_open (priv->control);
}

DalstonVolumeApplet *
dalston_volume_applet_new (MplPanelClient *panel_client)
{
  return g_object_new (DALSTON_TYPE_VOLUME_APPLET,
                       "panel-client", panel_client,
                       NULL);
}

GtkWidget *
dalston_volume_applet_get_pane (DalstonVolumeApplet *applet)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (applet);

  return priv->pane;
}



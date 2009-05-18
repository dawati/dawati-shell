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
#include "dalston-volume-status-icon.h"

G_DEFINE_TYPE (DalstonVolumeApplet, dalston_volume_applet, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_VOLUME_APPLET, DalstonVolumeAppletPrivate))

typedef struct _DalstonVolumeAppletPrivate DalstonVolumeAppletPrivate;

struct _DalstonVolumeAppletPrivate {
  GvcMixerControl *control;
  GtkStatusIcon *status_icon;
  GtkWidget *pane;
};

static void
dalston_volume_applet_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_volume_applet_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_volume_applet_dispose (GObject *object)
{
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

  g_type_class_add_private (klass, sizeof (DalstonVolumeAppletPrivate));

  object_class->get_property = dalston_volume_applet_get_property;
  object_class->set_property = dalston_volume_applet_set_property;
  object_class->dispose = dalston_volume_applet_dispose;
  object_class->finalize = dalston_volume_applet_finalize;
}

static void
_mixer_control_ready_cb (GvcMixerControl *control,
                         gpointer         userdata)
{
  g_debug (G_STRLOC ": Mixer ready!");
}

static void
_mixer_control_default_sink_changed_cb (GvcMixerControl *control,
                                        guint            id,
                                        gpointer         userdata)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (userdata);

  g_object_set (priv->status_icon,
                "sink",
                gvc_mixer_control_get_default_sink (control),
                NULL);

  g_object_set (priv->pane,
                "sink",
                gvc_mixer_control_get_default_sink (control),
                NULL);
}

static void
dalston_volume_applet_init (DalstonVolumeApplet *self)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (self);

  priv->status_icon = dalston_volume_status_icon_new ();
  priv->pane = dalston_volume_pane_new ();

  priv->control = gvc_mixer_control_new ();
  g_signal_connect (priv->control,
                    "default-sink-changed",
                    (GCallback)_mixer_control_default_sink_changed_cb,
                    self);
  g_signal_connect (priv->control,
                    "ready",
                    (GCallback)_mixer_control_ready_cb,
                    self);
  gvc_mixer_control_open (priv->control);
}

DalstonVolumeApplet *
dalston_volume_applet_new (void)
{
  return g_object_new (DALSTON_TYPE_VOLUME_APPLET, NULL);
}

GtkStatusIcon *
dalston_volume_applet_get_status_icon (DalstonVolumeApplet *applet)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (applet);

  return priv->status_icon;
}

GtkWidget *
dalston_volume_applet_get_pane (DalstonVolumeApplet *applet)
{
  DalstonVolumeAppletPrivate *priv = GET_PRIVATE (applet);

  return priv->pane;
}



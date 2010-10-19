
/*
 * Copyright © 2011 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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

#include <math.h>

#include <gconf/gconf-client.h>

#include "mpd-conf.h"
#include "mpd-gobject.h"
#include "config.h"

G_DEFINE_TYPE (MpdConf, mpd_conf, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_CONF, MpdConfPrivate))

enum
{
  PROP_0,

  PROP_BRIGHTNESS_ENABLED,
  PROP_BRIGHTNESS_VALUE,
  PROP_BRIGHTNESS_VALUE_BATTERY,
  PROP_LID_ACTION,
  PROP_SUSPEND_IDLE_TIME
};

typedef struct
{
  GConfClient   *client;
  unsigned int   client_connection_id;
} MpdConfPrivate;

#define MPD_CONF_DIR "/desktop/meego"
#define MPD_CONF_BRIGHTNESS_ENABLED       MPD_CONF_DIR"/panel-devices/brightness_enabled"
#define MPD_CONF_BRIGHTNESS_VALUE         MPD_CONF_DIR"/panel-devices/brightness_value"
#define MPD_CONF_BRIGHTNESS_VALUE_BATTERY MPD_CONF_DIR"/panel-devices/brightness_value_battery"
#define MPD_CONF_LID_ACTION               MPD_CONF_DIR"/panel-devices/buttons/lid"
#define MPD_CONF_SUSPEND_IDLE_TIME        MPD_CONF_DIR"/panel-devices/timeout/sleep_computer"

#define MPD_CONF_MIN_SUSPEND_DELAY  15

static void
_gconf_mpd_notify_cb (GConfClient   *client,
                      unsigned int   connection_id,
                      GConfEntry    *entry,
                      MpdConf       *self)
{
  char const  *key;
  GConfValue  *value;

  key = gconf_entry_get_key (entry);
  value = gconf_entry_get_value (entry);

  if (0 == g_strcmp0 (key, MPD_CONF_SUSPEND_IDLE_TIME))
  {
    g_object_notify (G_OBJECT (self), "suspend-idle-time");
  }

  if (0 == g_strcmp0 (key, MPD_CONF_BRIGHTNESS_ENABLED))
  {
    g_object_notify (G_OBJECT (self), "brightness-enabled");
  }

  if (0 == g_strcmp0 (key, MPD_CONF_BRIGHTNESS_VALUE))
  {
    g_object_notify (G_OBJECT (self), "brightness-value");
  }

  if (0 == g_strcmp0 (key, MPD_CONF_BRIGHTNESS_VALUE_BATTERY))
  {
    g_object_notify (G_OBJECT (self), "brightness-value-battery");
  }

  if (0 == g_strcmp0 (key, MPD_CONF_LID_ACTION))
  {
    g_object_notify (G_OBJECT (self), "lid-action");
  }
}

static void
_get_property (GObject      *object,
               unsigned int  property_id,
               GValue       *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_BRIGHTNESS_ENABLED:
    g_value_set_boolean (value,
                         mpd_conf_get_brightness_enabled (MPD_CONF (object)));
    break;
  case PROP_BRIGHTNESS_VALUE:
    g_value_set_float (value,
                       mpd_conf_get_brightness_value (MPD_CONF (object)));
    break;
  case PROP_LID_ACTION:
    g_value_set_uint (value,
                      mpd_conf_get_lid_action (MPD_CONF (object)));
    break;
  case PROP_SUSPEND_IDLE_TIME:
    g_value_set_int (value,
                     mpd_conf_get_suspend_idle_time (MPD_CONF (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               unsigned int  property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_BRIGHTNESS_VALUE:
    mpd_conf_set_brightness_value (MPD_CONF (object),
                                   g_value_get_float (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdConfPrivate *priv = GET_PRIVATE (object);

  if (priv->client_connection_id)
  {
    gconf_client_notify_remove (priv->client, priv->client_connection_id);
    priv->client_connection_id = 0;
  }

  mpd_gobject_detach (object, (GObject **) &priv->client);

  G_OBJECT_CLASS (mpd_conf_parent_class)->dispose (object);
}

static GObject *
_constructor (GType                  type,
              unsigned int           n_properties,
              GObjectConstructParam *properties)
{
  static GObject *self = NULL;

  if (self == NULL)
  {
    self = G_OBJECT_CLASS (mpd_conf_parent_class)
              ->constructor (type, n_properties, properties);
    g_object_add_weak_pointer (self, (gpointer) &self);
  } else {
    g_object_ref (self);
  }

  return self;
}

static void
mpd_conf_class_init (MpdConfClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdConfPrivate));

  object_class->constructor = _constructor;
  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;

  /* Properties */

  param_flags = G_PARAM_READABLE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_BRIGHTNESS_ENABLED,
                                   g_param_spec_boolean ("brightness-enabled",
                                                         "Brightness enabled",
                                                         "If display brightness control is enabled",
                                                         false,
                                                         param_flags));

  g_object_class_install_property (object_class,
                                   PROP_BRIGHTNESS_VALUE,
                                   g_param_spec_float ("brightness-value",
                                                       "Brightness value",
                                                       "Display brightness value",
                                                       0.0, 1.0, 1.0,
                                                       param_flags | G_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
                                   PROP_BRIGHTNESS_VALUE_BATTERY,
                                   g_param_spec_float ("brightness-value-battery",
                                                       "Brightness value on battery",
                                                       "Display brightness value on battery",
                                                       0.0, 1.0, 1.0,
                                                       param_flags | G_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
                                   PROP_LID_ACTION,
                                   g_param_spec_uint ("lid-action",
                                                      "Lid action",
                                                      "Action when lid is closed",
                                                      MPD_CONF_LID_ACTION_NONE,
                                                      MPD_CONF_LID_ACTION_SUSPEND,
                                                      MPD_CONF_LID_ACTION_SUSPEND,
                                                      param_flags));

  g_object_class_install_property (object_class,
                                   PROP_SUSPEND_IDLE_TIME,
                                   g_param_spec_int ("suspend-idle-time",
                                                     "Suspend idle time",
                                                     "Time to suspend when idle",
                                                     -1,
                                                     3600, /* whatever */
                                                     30,
                                                     param_flags));
}

static void
mpd_conf_init (MpdConf *self)
{
  MpdConfPrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;

  priv->client = gconf_client_get_default ();
  gconf_client_add_dir (priv->client,
                        MPD_CONF_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  } else
  {
    priv->client_connection_id = gconf_client_notify_add (
                                  priv->client,
                                  MPD_CONF_DIR,
                                  (GConfClientNotifyFunc) _gconf_mpd_notify_cb,
                                  self,
                                  NULL,
                                  &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
  }

}

MpdConf *
mpd_conf_new (void)
{
  return g_object_new (MPD_TYPE_CONF, NULL);
}

bool
mpd_conf_get_brightness_enabled (MpdConf *self)
{
  MpdConfPrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;
  bool     enabled;

  g_return_val_if_fail (MPD_IS_CONF (self), false);

  enabled = gconf_client_get_bool (priv->client,
                                   MPD_CONF_BRIGHTNESS_ENABLED,
                                   &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  return enabled;
}

float
mpd_conf_get_brightness_value (MpdConf *self)
{
  MpdConfPrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;
  float    brightness;

  g_return_val_if_fail (MPD_IS_CONF (self), -1);

  brightness = gconf_client_get_float (priv->client,
                                       MPD_CONF_BRIGHTNESS_VALUE,
                                       &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  return CLAMP (brightness, 0, 1);
}

void
mpd_conf_set_brightness_value (MpdConf *self,
                               float    brightness)
{
  MpdConfPrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;
  float    old_brightness;

  old_brightness = mpd_conf_get_brightness_value (self);
  if (fabs (brightness - old_brightness) >= 0.01)
  {
    gconf_client_set_float (priv->client,
                            MPD_CONF_BRIGHTNESS_VALUE,
                            brightness,
                            &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    } else {
      g_object_notify (G_OBJECT (self), "brightness-value");
    }
  }
}

float
mpd_conf_get_brightness_value_battery (MpdConf *self)
{
  MpdConfPrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;
  float    brightness;

  g_return_val_if_fail (MPD_IS_CONF (self), -1);

  brightness = gconf_client_get_float (priv->client,
                                       MPD_CONF_BRIGHTNESS_VALUE_BATTERY,
                                       &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  return CLAMP (brightness, 0, 1);
}

void
mpd_conf_set_brightness_value_battery (MpdConf *self,
                                       float    brightness)
{
  MpdConfPrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;
  float    old_brightness;

  old_brightness = mpd_conf_get_brightness_value_battery (self);
  if (fabs (brightness - old_brightness) >= 0.01)
  {
    gconf_client_set_float (priv->client,
                            MPD_CONF_BRIGHTNESS_VALUE_BATTERY,
                            brightness,
                            &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    } else {
      g_object_notify (G_OBJECT (self), "brightness-value-battery");
    }
  }
}

int
mpd_conf_get_suspend_idle_time (MpdConf *self)
{
  MpdConfPrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;
  int      suspend_idle_time;

  g_return_val_if_fail (MPD_IS_CONF (self), -1);

  suspend_idle_time = gconf_client_get_int (priv->client,
                                            MPD_CONF_SUSPEND_IDLE_TIME,
                                            &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  /* Suspend now but leave a few seconds for gnome-screensaver:
   * otherwise may get a screensave _after_ resuming. This
   * also lets screensaver finish any fading it wants to do,
   * not to mention giving the user a few seconds to break idle */
  if (suspend_idle_time > 0 && suspend_idle_time < MPD_CONF_MIN_SUSPEND_DELAY)
    suspend_idle_time = MPD_CONF_MIN_SUSPEND_DELAY;

  return suspend_idle_time;
}

MpdConfLidAction
mpd_conf_get_lid_action (MpdConf *self)
{
  MpdConfPrivate *priv = GET_PRIVATE (self);
  char              *action;
  MpdConfLidAction   lid_action = MPD_CONF_LID_ACTION_SUSPEND;
  GError            *error = NULL;

  g_return_val_if_fail (MPD_IS_CONF (self), MPD_CONF_LID_ACTION_SUSPEND);

  action = gconf_client_get_string (priv->client,
                                    MPD_CONF_LID_ACTION,
                                    &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  } else {
    if (0 == g_strcmp0 ("suspend", action))
      lid_action = MPD_CONF_LID_ACTION_SUSPEND;
    else if (0 == g_strcmp0 ("none", action))
      lid_action = MPD_CONF_LID_ACTION_NONE;
  }

  /* FIXME: maybe clamp to property boundaries? */
  return lid_action;
}


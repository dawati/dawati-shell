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


#include "dalston-brightness-manager.h"

#include <libhal-glib/hal-manager.h>
#include <libhal-glib/hal-device.h>

#include <libhal-panel-glib/hal-panel-proxy.h>

G_DEFINE_TYPE (DalstonBrightnessManager, dalston_brightness_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_BRIGHTNESS_MANAGER, DalstonBrightnessManagerPrivate))

typedef struct _DalstonBrightnessManagerPrivate DalstonBrightnessManagerPrivate;

struct _DalstonBrightnessManagerPrivate {
  HalManager    *manager;
  HalDevice     *panel_device;
  gchar         *panel_udi;
  HalPanelProxy *panel_proxy;
  gboolean       is_hal_running;

  guint num_levels_discover_idle;
  guint monitoring_timeout;
  gint num_levels;

  gint previous_brightness;
  gboolean controllable;
};

enum
{
  NUM_LEVELS_CHANGED,
  BRIGHTNESS_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

#define MONITORING_MILLISECONDS_TIMEOUT 500

static void
dalston_brightness_manager_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_brightness_manager_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose_panel_device (GObject *object)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (object);

  if (priv->panel_device)
  {
    g_object_unref (priv->panel_device);
    priv->panel_device = NULL;
  }

  if (priv->num_levels_discover_idle)
  {
    g_source_remove (priv->num_levels_discover_idle);
    priv->num_levels_discover_idle = 0;
  }

  if (priv->monitoring_timeout)
  {
    g_source_remove (priv->monitoring_timeout);
    priv->monitoring_timeout = 0;
  }

  if (priv->panel_proxy)
  {
    g_object_unref (priv->panel_proxy);
    priv->panel_proxy = NULL;
  }
}

static void
dalston_brightness_manager_dispose (GObject *object)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (object);

  if (priv->manager)
  {
    g_object_unref (priv->manager);
    priv->manager = NULL;
  }

  _dispose_panel_device (object);

  G_OBJECT_CLASS (dalston_brightness_manager_parent_class)->dispose (object);
}

static void
dalston_brightness_manager_finalize (GObject *object)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (object);

  g_free (priv->panel_udi);

  G_OBJECT_CLASS (dalston_brightness_manager_parent_class)->finalize (object);
}

static void
dalston_brightness_manager_num_levels_changed (DalstonBrightnessManager *manager,
                                               gint                      num_levels)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (manager);

  priv->num_levels = num_levels;
}

static void
dalston_brightness_manager_class_init (DalstonBrightnessManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DalstonBrightnessManagerPrivate));

  object_class->get_property = dalston_brightness_manager_get_property;
  object_class->set_property = dalston_brightness_manager_set_property;
  object_class->dispose = dalston_brightness_manager_dispose;
  object_class->finalize = dalston_brightness_manager_finalize;

  klass->num_levels_changed = dalston_brightness_manager_num_levels_changed;

  signals[NUM_LEVELS_CHANGED] = 
    g_signal_new ("num-levels-changed",
                  DALSTON_TYPE_BRIGHTNESS_MANAGER,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET(DalstonBrightnessManagerClass, num_levels_changed),
                  0,
                  NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);

  signals[BRIGHTNESS_CHANGED] =
    g_signal_new ("brightness-changed",
                  DALSTON_TYPE_BRIGHTNESS_MANAGER,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET(DalstonBrightnessManagerClass, brightness_changed),
                  0,
                  NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);
}

static gboolean
_num_levels_discover_idle_cb (gpointer data)
{
  DalstonBrightnessManager *manager = (DalstonBrightnessManager *)data;
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (data);
  gint value = -1;
  GError *error = NULL;

  if (!hal_device_get_int (priv->panel_device,
                           "laptop_panel.num_levels",
                           &value,
                           &error))
  {
    g_warning (G_STRLOC ": Error getting number of brightness levels: %s",
               error->message);
    g_clear_error (&error);
  } else {
    g_signal_emit (manager, signals[NUM_LEVELS_CHANGED], 0, value);
  }

  priv->num_levels_discover_idle = 0;

  return FALSE;
}

static void
_panel_proxy_get_brightness_cb (HalPanelProxy *proxy,
                                gint           value,
                                const GError  *error,
                                GObject       *weak_object,
                                gpointer       userdata)
{
  DalstonBrightnessManager *manager = (DalstonBrightnessManager *)weak_object;
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (weak_object);

  if (error)
  {
    g_warning (G_STRLOC ": Error querying brightness: %s",
               error->message);
    g_warning (G_STRLOC ": Stopping monitoring");
    dalston_brightness_manager_stop_monitoring (manager);
  }

  if (priv->previous_brightness != value)
  {
    priv->previous_brightness = value;
    g_signal_emit (weak_object,
                   signals[BRIGHTNESS_CHANGED], 
                   0,
                   value);
  }
}

static void
_setup_panel_device (DalstonBrightnessManager *self)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (self);
  gchar **names;
  GError *error = NULL;

  if (!hal_manager_find_capability (priv->manager,
                                    "laptop_panel",
                                    &names,
                                    &error))
  {
    g_warning (G_STRLOC ": Unable to find device with capability 'laptop_panel': %s",
               error->message);
    g_clear_error (&error);
    return;
  }

  priv->panel_udi = g_strdup (names[0]);
  hal_manager_free_capability (names);

  if (!priv->panel_udi)
  {
    return;
  }

  priv->panel_device = hal_device_new ();
  hal_device_set_udi (priv->panel_device, priv->panel_udi);

  priv->num_levels_discover_idle = g_idle_add (_num_levels_discover_idle_cb, 
                                               self);

  priv->panel_proxy = hal_panel_proxy_new (priv->panel_udi);

  if (!priv->panel_proxy)
  {
    g_warning (G_STRLOC ": Unable to get panel proxy for %s", priv->panel_udi);
    return;
  }

  hal_panel_proxy_get_brightness_async (priv->panel_proxy,
                                        _panel_proxy_get_brightness_cb,
                                        (GObject *)self,
                                        NULL);

  priv->controllable = TRUE;
}

static void
_cleanup_panel_device (DalstonBrightnessManager *self)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (self);

  _dispose_panel_device (G_OBJECT (self));
  g_free (priv->panel_udi);
}

static void
_hal_daemon_start (HalManager               *manager,
                   DalstonBrightnessManager *self)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (self);

  /* spurious signal ? */
  if (priv->is_hal_running)
    return;

  _setup_panel_device (self);

  priv->is_hal_running = TRUE;
}

static void
_hal_daemon_stop (HalManager               *manager,
                  DalstonBrightnessManager *self)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (self);

  /* spurious signal ? */
  if (!priv->is_hal_running)
    return;

  _cleanup_panel_device (self);

  priv->is_hal_running = FALSE;
}

static void
dalston_brightness_manager_init (DalstonBrightnessManager *self)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (self);

  priv->previous_brightness = -1;

  priv->manager = hal_manager_new ();
  g_signal_connect (priv->manager,
                    "daemon-start",
                    G_CALLBACK (_hal_daemon_start),
                    self);
  g_signal_connect (priv->manager,
                    "daemon-stop",
                    G_CALLBACK(_hal_daemon_stop),
                    self);

  priv->is_hal_running = FALSE;
  if (hal_manager_is_running (priv->manager))
  {
    priv->is_hal_running = TRUE;
    _setup_panel_device (self);
  }

}

DalstonBrightnessManager *
dalston_brightness_manager_new (void)
{
  return g_object_new (DALSTON_TYPE_BRIGHTNESS_MANAGER, NULL);
}

static void
dalston_brightness_manager_get_brightness (DalstonBrightnessManager *manager)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (manager);

  if (!priv->panel_proxy)
    return;

  hal_panel_proxy_get_brightness_async (priv->panel_proxy,
                                        _panel_proxy_get_brightness_cb,
                                        (GObject *)manager,
                                        NULL);
}

static gboolean
_brightness_monitoring_timeout_cb (gpointer data)
{
  DalstonBrightnessManager *manager = (DalstonBrightnessManager *)data;
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (manager);

  if (!priv->panel_proxy)
    return FALSE;

  dalston_brightness_manager_get_brightness (manager);

  return TRUE;
}

void
dalston_brightness_manager_start_monitoring (DalstonBrightnessManager *manager)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (manager);

  if (priv->monitoring_timeout)
  {
    g_warning (G_STRLOC ": Monitoring already running.");
    return;
  }

  priv->monitoring_timeout =
    g_timeout_add (MONITORING_MILLISECONDS_TIMEOUT,
                   _brightness_monitoring_timeout_cb,
                   manager);

  if (priv->panel_proxy)
    dalston_brightness_manager_get_brightness (manager);
}

void
dalston_brightness_manager_stop_monitoring (DalstonBrightnessManager *manager)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (manager);

  if (!priv->monitoring_timeout)
  {
    g_warning (G_STRLOC ": Monitoring not running.");
    return;
  }

  g_source_remove (priv->monitoring_timeout);
  priv->monitoring_timeout = 0;
}

static void
_panel_proxy_set_brightness_cb (HalPanelProxy *proxy,
                                const GError  *error,
                                GObject       *weak_object,
                                gpointer       userdata)
{
  if (error)
  {
    g_warning (G_STRLOC ": Error setting brightness: %s",
               error->message);
  }
}

void
dalston_brightness_manager_set_brightness (DalstonBrightnessManager *manager,
                                           gint                      value)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (manager);

  if (!priv->panel_proxy)
    return;

  hal_panel_proxy_set_brightness_async (priv->panel_proxy,
                                        value,
                                        _panel_proxy_set_brightness_cb,
                                        (GObject *)manager,
                                        NULL);
}

gboolean
dalston_brightness_manager_is_controllable (DalstonBrightnessManager *manager)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (manager);

  return priv->controllable;
}

/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Author: Jussi Kukkonen <jussi.kukkonen@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <glib/gi18n.h>

#include "dawati-bt-device.h"

G_DEFINE_TYPE (DawatiBtDevice, dawati_bt_device, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DAWATI_TYPE_BT_DEVICE, DawatiBtDevicePrivate))

typedef struct _DawatiBtDevicePrivate DawatiBtDevicePrivate;

struct _DawatiBtDevicePrivate {
  ClutterActor *label;

  char *name;
  char *device_path;
  gboolean connected;
};

enum {
  SIGNAL_DISCONNECT,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

enum
{
  PROP_0,
  PROP_NAME,
  PROP_DEVICE_PATH,
  PROP_CONNECTED,
};

static void
dawati_bt_device_set_name (DawatiBtDevice *device, const char *name)
{
  DawatiBtDevicePrivate *priv = GET_PRIVATE (device);
  char* label;

  g_free (priv->name);
  priv->name = g_strdup (name);

  label = g_strdup_printf (_("Connected to %s"), priv->name);
  mx_label_set_text (MX_LABEL (priv->label), label);
  g_free (label);
}

static void
dawati_bt_device_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  DawatiBtDevicePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_DEVICE_PATH:
      g_value_set_string (value, priv->device_path);
      break;
    case PROP_CONNECTED:
      g_value_set_boolean (value, priv->connected);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dawati_bt_device_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  DawatiBtDevicePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_NAME:
      dawati_bt_device_set_name (DAWATI_BT_DEVICE (object),
                                 g_value_get_string (value));
      break;
    case PROP_DEVICE_PATH:
      g_free (priv->device_path);
      priv->device_path = g_value_dup_string (value);
      break;
    case PROP_CONNECTED:
      priv->connected = g_value_get_boolean (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dawati_bt_device_dispose (GObject *object)
{
  DawatiBtDevicePrivate *priv = GET_PRIVATE (object);

  g_free (priv->device_path);
  priv->device_path = NULL;

  G_OBJECT_CLASS (dawati_bt_device_parent_class)->dispose (object);
}

static void
dawati_bt_device_class_init (DawatiBtDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (DawatiBtDevicePrivate));

  object_class->get_property = dawati_bt_device_get_property;
  object_class->set_property = dawati_bt_device_set_property;
  object_class->dispose = dawati_bt_device_dispose;

  signals[SIGNAL_DISCONNECT] = g_signal_new ("disconnect",
                                             G_TYPE_FROM_CLASS (object_class),
                                             G_SIGNAL_RUN_FIRST,
                                             G_STRUCT_OFFSET (DawatiBtDeviceClass, disconnect),
                                             NULL, NULL,
                                             g_cclosure_marshal_VOID__VOID,
                                             G_TYPE_NONE,
                                             0);

  pspec = g_param_spec_string ("name",
                               "name",
                               "Device name",
                               "",
                               G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_NAME, pspec);

  pspec = g_param_spec_string ("device-path",
                               "device-path",
                               "Bluetooth Device D-Bus path",
                               "",
                               G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DEVICE_PATH, pspec);

  pspec = g_param_spec_boolean ("connected",
                                "connected",
                                "Is the device connected",
                                TRUE,
                                G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONNECTED, pspec);
}

static void
_disconnect_clicked_cb (MxButton *button, DawatiBtDevice *device)
{
  g_signal_emit (device, signals[SIGNAL_DISCONNECT], 0);
}

static void
dawati_bt_device_init (DawatiBtDevice *device)
{
  ClutterActor *button;
  DawatiBtDevicePrivate *priv = GET_PRIVATE (device);

  mx_stylable_set_style_class (MX_STYLABLE (device), "BtDevice");

  priv->label = mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->label), "BtTitle");
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (device), priv->label, -1,
                                              "expand", TRUE,
                                              "x-fill", FALSE,
                                              "y-fill", FALSE,
                                              "x-align", MX_ALIGN_START,
                                              "y-align", MX_ALIGN_MIDDLE,
                                              NULL);

  button = mx_button_new ();
  mx_stylable_set_style_class (MX_STYLABLE (button), "BtCloseButton");
  mx_bin_set_child (MX_BIN (button), mx_icon_new ());
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_disconnect_clicked_cb), device);
  mx_box_layout_insert_actor (MX_BOX_LAYOUT (device), button, -1);
}


ClutterActor*
dawati_bt_device_new (const char *name, const char *device_path)
{
  return g_object_new (DAWATI_TYPE_BT_DEVICE,
                       "name", name,
                       "device-path", device_path,
                       "connected", TRUE,
                       NULL);
}


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

#include <stdlib.h>
#include <glib/gi18n.h>
#include <libnotify/notify.h>

#include "bluetooth-applet.h"

#include "dawati-bt-shell.h"
#include "dawati-bt-device.h"
#include "dawati-bt-request.h"


/* This is a UI mostly for gnome-bluetooth functionality, with one exception:
 * Toggling the Bluetooth-switch actually matches Connman Technology state
 * (because Connman does not allow anyone else to modify rfkill devices)
 */

/* TRANSLATORS: Title for a notification */
static const char *pairing_title = N_("New Bluetooth device");
static const char *icon = "bluetooth";

G_DEFINE_TYPE (DawatiBtShell, dawati_bt_shell, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DAWATI_TYPE_BT_SHELL, DawatiBtShellPrivate))

typedef struct _DawatiBtShellPrivate DawatiBtShellPrivate;

struct _DawatiBtShellPrivate {
  ClutterActor *kill_toggle;
  gulong        kill_handler;

  ClutterActor *request_box;
  ClutterActor *device_panelbox;
  ClutterActor *device_box;
  ClutterActor *info_label;
  ClutterActor *send_button;
  ClutterActor *add_button;

  GHashTable *devices;
  GHashTable *requests;


  MplPanelClient *panel_client;

  BluetoothApplet *applet;
  gboolean discoverable;

  GDBusProxy *connman;
  gboolean available;
  gboolean enabled;

  NotifyNotification *notification;
};

enum
{
  PROP_0,
  PROP_PANEL_CLIENT
};

static void dawati_bt_shell_launch (DawatiBtShell *shell, const char *command_line);
static void dawati_bt_shell_update (DawatiBtShell *shell);
static void dawati_bt_shell_set_connman (DawatiBtShell *shell, GDBusProxy *proxy);
static void dawati_bt_shell_handle_connman_property (DawatiBtShell *shell, const char *key, GVariant *value);
static ClutterActor* dawati_bt_shell_add_device (DawatiBtShell *shell, const char *name, const char *device_path);
static void dawati_bt_shell_add_request (DawatiBtShell *shell, const char *name, const char *device_path, DawatiBtRequestType type, const char *data);

static void
_bluetooth_simple_device_free (gpointer boxed)
{
  BluetoothSimpleDevice* obj = (BluetoothSimpleDevice*) boxed;

  g_free (obj->device_path);
  g_free (obj->bdaddr);
  g_free (obj->alias);
  g_free (obj);
}

static void
_remove_non_connected_device (ClutterActor *actor, DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  gboolean connected;
  char *path;

  g_object_get (actor,
                "connected", &connected,
                "device-path", &path,
                NULL);
  if (connected) {
    /* clear value for next round */
    g_object_set (actor, "connected", FALSE, NULL);
  } else {
    g_hash_table_remove (priv->devices, path);
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->device_box),
                                    actor);
  }
}

static void
_device_widget_disconnect_cb (DawatiBtDevice *device, DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  char *path;

  g_object_get (device, "device-path", &path, NULL);
  bluetooth_applet_disconnect_device (priv->applet, path, NULL, NULL);
}

static void
_request_response_cb (DawatiBtRequest *request,
                      DawatiBtResponse response,
                      const char *response_data,
                      DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  DawatiBtRequestType type;
  const char *path;
  int passkey;

  g_object_get (request,
                "request-type", &type,
                "device-path", &path,
                NULL);
  switch (type) {
  case DAWATI_BT_REQUEST_TYPE_AUTH:
    bluetooth_applet_agent_reply_auth (priv->applet, path,
                                       response != DAWATI_BT_REQUEST_RESPONSE_NO,
                                       response == DAWATI_BT_REQUEST_RESPONSE_ALWAYS);
    break;
  case DAWATI_BT_REQUEST_TYPE_CONFIRM:
    bluetooth_applet_agent_reply_confirm (priv->applet, path,
                                          response != DAWATI_BT_REQUEST_RESPONSE_NO);
    break;
  case DAWATI_BT_REQUEST_TYPE_PASSKEY:
    /* if atoi() fails, it fails, not much we can do.. */
    if (!response_data || response == DAWATI_BT_REQUEST_RESPONSE_NO)
      passkey = -1;
    else
      passkey = atoi (response_data);
    bluetooth_applet_agent_reply_passkey (priv->applet, path, passkey);
    break;
  case DAWATI_BT_REQUEST_TYPE_PIN:
    if (response == DAWATI_BT_REQUEST_RESPONSE_NO)
      bluetooth_applet_agent_reply_pincode (priv->applet, path, NULL);
    else
      bluetooth_applet_agent_reply_pincode (priv->applet, path, response_data);
    break;
  default:
    g_warn_if_reached ();
  }

  clutter_container_remove_actor (CLUTTER_CONTAINER (priv->request_box),
                                  CLUTTER_ACTOR (request));
  g_hash_table_remove (priv->requests, path);
}

static void
_handle_device (BluetoothSimpleDevice *device,
                DawatiBtShell         *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  ClutterActor *dev_widget;

  dev_widget = g_hash_table_lookup (priv->devices, device->device_path);
  if (dev_widget) {
    /* update existing widget if found: non-connected  will get removed
     * later */
    g_object_set (dev_widget,
                  "name", device->alias,
                  "connected", device->connected,
                  NULL);
  } else if (device->connected) {
    dawati_bt_shell_add_device (shell, device->alias, device->device_path);
  }
}

static void
_discoverable_cb (BluetoothApplet *applet,
                  GParamSpec      *pspec,
                  DawatiBtShell   *shell)
{
  gboolean discoverable;
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);

  discoverable = bluetooth_applet_get_discoverable (applet);
  if (discoverable != priv->discoverable) {
    priv->discoverable = discoverable;
    dawati_bt_shell_update (shell);
  }
}

static void
_devices_changed_cb(BluetoothApplet *applet,
                    DawatiBtShell   *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  GList *devices;

  devices = bluetooth_applet_get_devices (applet);
  g_list_foreach (devices, (GFunc)_handle_device, shell);
  g_list_free_full (devices, _bluetooth_simple_device_free);

  clutter_container_foreach (CLUTTER_CONTAINER (priv->device_box),
                             (ClutterCallback)_remove_non_connected_device,
                             shell);

  dawati_bt_shell_update (shell);
}

static void
_connman_enable_bluetooth_cb (GDBusProxy *connman,
                              GAsyncResult *res,
                              DawatiBtShell *shell)
{
  GError *err = NULL;

  g_dbus_proxy_call_finish (connman, res, &err);
  if (err) {
    g_warning ("Failed to change RFKILL state using Connman: %s",
               err->message);
    g_error_free (err);

    /* update because switch may now be incorrect */
    dawati_bt_shell_update (shell);
  }
}

static void
_toggle_active_cb (MxToggle      *toggle,
                   GParamSpec    *pspec,
                   DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  gboolean active;

  active = mx_toggle_get_active (MX_TOGGLE (toggle));

  if (!priv->available ||
      (active && priv->enabled) ||
      (!active && !priv->enabled)) {
    return;
  }

  if (priv->connman) {
    g_dbus_proxy_call (priv->connman,
                       active ? "EnableTechnology" : "DisableTechnology",
                       g_variant_new ("(s)", "bluetooth"),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       (GAsyncReadyCallback)_connman_enable_bluetooth_cb,
                       shell);
  }
}

static void
_add_clicked_cb (MxButton *button, DawatiBtShell *shell)
{
  dawati_bt_shell_launch (shell, "bluetooth-wizard");
}

static void
_send_clicked_cb (MxButton *button, DawatiBtShell *shell)
{
  dawati_bt_shell_launch (shell, "bluetooth-sendto");
}

static void
_settings_clicked_cb (MxButton *button, DawatiBtShell *shell)
{
  dawati_bt_shell_launch (shell, "gnome-control-center bluetooth");
}

static void
_connman_proxy_new_for_bus_cb (GDBusProxy *connman,
                               GAsyncResult *res,
                               DawatiBtShell *shell)
{
  GError *err = NULL;
  GDBusProxy *proxy = NULL;

  proxy = g_dbus_proxy_new_for_bus_finish (res, &err);
  if (err) {
    g_warning ("No connman proxy: %s", err->message);
    g_error_free (err);
  }
  dawati_bt_shell_set_connman (shell, proxy);
}

static void
_connman_appeared_cb (GDBusConnection *connection,
                      const char *name,
                      const char *owner,
                      DawatiBtShell *shell)
{
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL, /* should add iface info here */
                            "net.connman",
                            "/",
                            "net.connman.Manager",
                            NULL,
                            (GAsyncReadyCallback)_connman_proxy_new_for_bus_cb,
                            shell);
}

static void
_connman_vanished_cb (GDBusConnection *connection,
                      const char *name,
                      DawatiBtShell *shell)
{
  dawati_bt_shell_set_connman (shell, NULL);
}

static void
_connman_signal_cb (GDBusProxy *connman,
                    gchar *sender_name,
                    gchar *signal_name,
                    GVariant *parameters,
                    DawatiBtShell *shell)
{
  GVariant *value;
  const char *key;

  if (g_strcmp0 (signal_name, "PropertyChanged") == 0) {
     g_variant_get (parameters, "(sv)", &key, &value);
     dawati_bt_shell_handle_connman_property (shell, key, value);
  }
}

static void
_connman_get_properties_cb (GDBusProxy *connman,
                            GAsyncResult *res,
                            DawatiBtShell *shell)
{
  GError *err = NULL;
  GVariant *props, *value;
  GVariantIter *iter;
  const char *key;

  props = g_dbus_proxy_call_finish (connman, res, &err);
  if (err) {
    g_warning ("Connman GetProperties failed: %s", err->message);
    g_error_free (err);
    return;
  }

  g_variant_get (props, "(a{sv})", &iter);
  while (g_variant_iter_loop (iter, "{sv}", &key, &value))
    dawati_bt_shell_handle_connman_property (shell, key, value);

  g_variant_iter_free (iter);
  g_variant_unref (props);
}

static void
_pincode_request_cb (BluetoothApplet *applet,
                     const char *path,
                     const char *name,
                     const char *long_name,
                     gboolean numeric,
                     DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  DawatiBtRequestType type;
  char *msg = NULL;
  ClutterActor *w;

  /* as strange as it seems, passkey is numeric, while pin is string.
   * See bluetooth_applet_agent_reply_*() */
  if (numeric)
    type = DAWATI_BT_REQUEST_TYPE_PASSKEY;
  else
    type = DAWATI_BT_REQUEST_TYPE_PIN;

  w = g_hash_table_lookup (priv->requests, path);
  if (w) {
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->request_box), w);
    g_hash_table_remove (priv->requests, path);
  }
  dawati_bt_shell_add_request (shell, name, path, type, NULL);

  /* TRANSLATORS: Notification
   * Placeholder: Bluetooth device name
   */
  msg = g_strdup_printf (_("%s would like to connect"), name);
  notify_notification_update (priv->notification,
                              _(pairing_title), msg, icon);
  notify_notification_show (priv->notification, NULL);
  g_free (msg);
}

static void
_confirm_request_cb (BluetoothApplet *applet,
                     const char *path,
                     const char *name,
                     const char *long_name,
                     guint pin,
                     DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  char *msg = NULL;
  ClutterActor *w;
  char *pin_str;

  w = g_hash_table_lookup (priv->requests, path);
  if (w) {
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->request_box), w);
    g_hash_table_remove (priv->requests, path);
  }

  /* according to BT spec, pin is 6 digits */
  pin_str = g_strdup_printf ("%06u", pin);
  dawati_bt_shell_add_request (shell, name, path,
                               DAWATI_BT_REQUEST_TYPE_CONFIRM, pin_str);
  g_free (pin_str);

  /* TRANSLATORS: Notification
   * Placeholder 1: Bluetooth device name
   */
  msg = g_strdup_printf (_("%s would like to confirm connection"), name);
  notify_notification_update (priv->notification,
                              _(pairing_title), msg, icon);
  notify_notification_show (priv->notification, NULL);
  g_free (msg);
}

static void
_auth_request_cb (BluetoothApplet *applet,
                  const char *path,
                  const char *name,
                  const char *long_name,
                  const char *uuid,
                  DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  char *msg = NULL;
  ClutterActor *w;

  w = g_hash_table_lookup (priv->requests, path);
  if (w) {
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->request_box), w);
    g_hash_table_remove (priv->requests, path);
  }

  dawati_bt_shell_add_request (shell, name, path,
                               DAWATI_BT_REQUEST_TYPE_AUTH, uuid);

  /* TRANSLATORS: Notification
   * Placeholder 1: Bluetooth device name
   */
  msg = g_strdup_printf (_("%s would like to access a service"), name);
  notify_notification_update (priv->notification,
                              _("New Bluetooth access request"), msg, icon);
  notify_notification_show (priv->notification, NULL);
  g_free (msg);
}

static void
_remove_child_from_container (ClutterActor *actor, ClutterContainer *cont)
{
  clutter_container_remove_actor (cont, actor);
}

static void
_cancel_request_cb (DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);

  clutter_container_foreach (CLUTTER_CONTAINER (priv->request_box),
                             (ClutterCallback)_remove_child_from_container,
                             priv->request_box);
  g_hash_table_remove_all (priv->requests);

  notify_notification_close (priv->notification, NULL);
}

static void
_notify_action_cb (NotifyNotification *notification,
                   char *action,
                   DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);

  mpl_panel_client_show (priv->panel_client);
}


static ClutterActor*
dawati_bt_shell_add_device (DawatiBtShell *shell, const char *name, const char *device_path)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  ClutterActor *w;

  w = dawati_bt_device_new (name, device_path);
  g_signal_connect (w, "disconnect",
                    G_CALLBACK (_device_widget_disconnect_cb), shell);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (priv->device_box), w, -1);
  g_hash_table_insert (priv->devices, g_strdup (device_path), w);

  return w;
}

static void
dawati_bt_shell_add_request (DawatiBtShell *shell,
                             const char *name,
                             const char *device_path,
                             DawatiBtRequestType type,
                             const char *data)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  ClutterActor *w;

  w = dawati_bt_request_new (name, device_path, type, data);
  g_signal_connect (w, "response",
                    G_CALLBACK (_request_response_cb), shell);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (priv->request_box), w, -1);
  g_hash_table_insert (priv->requests, g_strdup (device_path), w);
}

static void
dawati_bt_shell_handle_connman_property (DawatiBtShell *shell,
                                         const char *key,
                                         GVariant *value)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  const char **techs;
  gboolean val = FALSE;

  if (g_strcmp0 (key, "AvailableTechnologies") == 0) {
    techs = g_variant_get_strv (value, NULL);
    while (*techs) {
      if (g_strcmp0 (*techs, "bluetooth") == 0) {
        val = TRUE;
        break;
      }
      techs++;
    }
    if (val != priv->available) {
      priv->available = val;
      dawati_bt_shell_update (shell);
    }
  } else if (g_strcmp0 (key, "EnabledTechnologies") == 0) {
    techs = g_variant_get_strv (value, NULL);
    while (*techs) {
      if (g_strcmp0 (*techs, "bluetooth") == 0) {
        val = TRUE;
        break;
      }
      techs++;
    }
    if (val != priv->enabled) {
      priv->enabled = val;
      dawati_bt_shell_update (shell);
    }
  }
}

static void
dawati_bt_shell_set_connman (DawatiBtShell *shell,
                             GDBusProxy *proxy)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);

  if (priv->connman) {
    g_signal_handlers_disconnect_by_func (priv->connman,
                                          G_CALLBACK (_connman_signal_cb),
                                          shell);
    g_object_unref (priv->connman);
  }

  priv->connman = proxy;
  if (priv->connman) {
    g_signal_connect (priv->connman, "g-signal",
                      G_CALLBACK (_connman_signal_cb), shell);

    g_dbus_proxy_call (priv->connman,
                       "GetProperties",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       (GAsyncReadyCallback)_connman_get_properties_cb,
                       shell);
  }
}

static void
dawati_bt_shell_update (DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);
  gboolean showinfo;

  showinfo = g_hash_table_size (priv->devices) == 0;

  g_signal_handler_block (priv->kill_toggle, priv->kill_handler);
  mx_toggle_set_active (MX_TOGGLE (priv->kill_toggle), priv->enabled);
  g_signal_handler_unblock (priv->kill_toggle, priv->kill_handler);

  /* Now way to know from Connman:
  mx_widget_set_disabled (MX_WIDGET (priv->kill_toggle), disabled);
  */
  g_object_set (priv->info_label, "visible", showinfo, NULL);
  g_object_set (priv->device_panelbox, "visible", priv->enabled, NULL);
  g_object_set (priv->add_button, "visible", priv->enabled, NULL);
  g_object_set (priv->send_button, "visible", priv->enabled, NULL);

  if (priv->panel_client) {
    g_object_set (priv->kill_toggle, "disabled", FALSE, NULL);
    if (!priv->available) {
      mpl_panel_client_request_button_state (priv->panel_client,
                                             MNB_BUTTON_HIDDEN);
    } else {
      mpl_panel_client_request_button_state (priv->panel_client,
                                             MNB_BUTTON_NORMAL);
      if (priv->enabled)
          mpl_panel_client_request_button_style (priv->panel_client,
                                                 "state-idle");
      else
          mpl_panel_client_request_button_style (priv->panel_client,
                                                 "state-off");
    }
  } else {
    g_object_set (priv->kill_toggle, "disabled", !priv->available, NULL);
  }
}

static void
dawati_bt_shell_launch (DawatiBtShell *shell, const char *cmd)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);

  if (priv->panel_client) {
    if (mpl_panel_client_launch_application (priv->panel_client, cmd))
      mpl_panel_client_hide (priv->panel_client);
  } else {
    g_spawn_command_line_async (cmd, NULL);
  }
}

static void
dawati_bt_shell_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_PANEL_CLIENT:
      g_value_set_object (value, priv->panel_client);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dawati_bt_shell_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_PANEL_CLIENT:
      priv->panel_client = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dawati_bt_shell_dispose (GObject *object)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (object);

  dawati_bt_shell_set_connman (DAWATI_BT_SHELL (object), NULL);

  if (priv->panel_client)
    g_object_unref (priv->panel_client);
  priv->panel_client = NULL;

  if (priv->devices)
    g_hash_table_unref (priv->devices);
  priv->devices = NULL;

  G_OBJECT_CLASS (dawati_bt_shell_parent_class)->dispose (object);
}


static void
dawati_bt_shell_class_init (DawatiBtShellClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (DawatiBtShellPrivate));

  object_class->get_property = dawati_bt_shell_get_property;
  object_class->set_property = dawati_bt_shell_set_property;
  object_class->dispose = dawati_bt_shell_dispose;

  pspec = g_param_spec_object ("panel-client",
                               "Panel client",
                               "Panel client",
                               MPL_TYPE_PANEL_CLIENT,
                               G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_PANEL_CLIENT, pspec);
}

static void
dawati_bt_shell_init_applet (DawatiBtShell *shell)
{
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);

  priv->applet = g_object_new (BLUETOOTH_TYPE_APPLET, NULL);

  g_signal_connect (priv->applet, "notify::discoverable",
                    G_CALLBACK (_discoverable_cb), shell);
  _discoverable_cb (priv->applet, NULL, shell);

  g_signal_connect (priv->applet, "devices-changed",
                    G_CALLBACK (_devices_changed_cb), shell);
  _devices_changed_cb (priv->applet, shell);

  g_signal_connect (priv->applet, "pincode-request",
                    G_CALLBACK (_pincode_request_cb), shell);
  g_signal_connect (priv->applet, "confirm-request",
                    G_CALLBACK (_confirm_request_cb), shell);
  g_signal_connect (priv->applet, "auth-request",
                    G_CALLBACK (_auth_request_cb), shell);
  g_signal_connect (priv->applet, "cancel-request",
                    G_CALLBACK (_cancel_request_cb), shell);
}

static void
dawati_bt_shell_init (DawatiBtShell *shell)
{
  ClutterActor *box, *label, *hbox, *button_box, *settings_button;
  DawatiBtShellPrivate *priv = GET_PRIVATE (shell);

  priv->devices = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, NULL);
  priv->requests = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, NULL);

  priv->notification = notify_notification_new ("", NULL, icon);
  notify_notification_set_timeout (priv->notification,
                                   NOTIFY_EXPIRES_NEVER);
  /* TRANSLATORS: button in a notification, will open the
   * bluetooth panel */
  notify_notification_add_action (priv->notification,
                                  "show",
                                  _("Show"),
                                  (NotifyActionCallback)_notify_action_cb,
                                  shell, NULL);

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (shell),
                                 MX_ORIENTATION_VERTICAL);
  mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (shell), TRUE);

  label = mx_label_new_with_text (_("Bluetooth"));
  mx_stylable_set_style_class (MX_STYLABLE (label), "titleBar");
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (shell), label, -1,
                                           "expand", TRUE,
                                           "x-fill", TRUE,
                                           "x-align", MX_ALIGN_START,
                                           "y-fill", FALSE,
                                           NULL);

  box = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box), MX_ORIENTATION_VERTICAL);
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (shell),
                                           box,
                                           -1,
                                           "expand", TRUE,
                                           "y-fill", TRUE,
                                           NULL);

  priv->kill_toggle = mx_toggle_new ();
  priv->kill_handler = g_signal_connect (priv->kill_toggle, "notify::active",
                                         G_CALLBACK (_toggle_active_cb), shell);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (box), priv->kill_toggle, -1);

  /* devices that are requesting something go here */
  priv->request_box = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->request_box),
                                 MX_ORIENTATION_VERTICAL);
  mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (priv->request_box),
                                       TRUE);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (box), priv->request_box, -1);


  /* connected devices go here */
  priv->device_panelbox = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->device_panelbox),
                                 MX_ORIENTATION_VERTICAL);
  mx_stylable_set_style_class (MX_STYLABLE (priv->device_panelbox), "contentPanel");
  mx_box_layout_add_actor (MX_BOX_LAYOUT (box), priv->device_panelbox, -1);

  priv->info_label = mx_label_new_with_text (_("Nothing connected"));
  clutter_actor_hide (priv->info_label);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (priv->device_panelbox), priv->info_label, -1);

  priv->device_box = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->device_box),
                                 MX_ORIENTATION_VERTICAL);
  mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (priv->device_box),
                                       TRUE);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (priv->device_panelbox), priv->device_box, -1);

  /* button row */
  hbox = mx_box_layout_new ();
  mx_box_layout_add_actor (MX_BOX_LAYOUT (box), hbox, -1);

  button_box = mx_box_layout_new ();
  mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (button_box), TRUE);
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (hbox), button_box, -1,
                                           "expand", TRUE,
                                           "x-fill", FALSE,
                                           "x-align", MX_ALIGN_END,
                                           NULL);

  priv->send_button = mx_button_new_with_label (_("Send file"));
  clutter_actor_hide (priv->send_button);
  g_signal_connect (priv->send_button, "clicked",
                    G_CALLBACK (_send_clicked_cb), shell);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (button_box), priv->send_button, -1);

  priv->add_button = mx_button_new_with_label (_("Add new"));
  clutter_actor_hide (priv->add_button);
  g_signal_connect (priv->add_button, "clicked",
                    G_CALLBACK (_add_clicked_cb), shell);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (button_box), priv->add_button, -1);

  settings_button = mx_button_new_with_label (_("Settings"));
  g_signal_connect (settings_button, "clicked",
                    G_CALLBACK (_settings_clicked_cb), shell);
  mx_box_layout_add_actor (MX_BOX_LAYOUT (button_box), settings_button, -1);

  dawati_bt_shell_init_applet (shell);

  g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                    "net.connman",
                    G_BUS_NAME_WATCHER_FLAGS_NONE,
                    (GBusNameAppearedCallback)_connman_appeared_cb,
                    (GBusNameVanishedCallback)_connman_vanished_cb,
                    shell, NULL);

#if 0
  dawati_bt_shell_add_device (shell, "TestDeviceA", "a");
  dawati_bt_shell_add_device (shell, "TestDeviceB", "b");
  dawati_bt_shell_add_request (shell, "TestDeviceE", "e", DAWATI_BT_REQUEST_TYPE_CONFIRM, "001234");
  dawati_bt_shell_add_request (shell, "TestDeviceE", "e", DAWATI_BT_REQUEST_TYPE_AUTH, "0000111f-0000-1000-8000-00805f9b34fb");
#endif
}

ClutterActor*
dawati_bt_shell_new (MplPanelClient *panel)
{
  return g_object_new (DAWATI_TYPE_BT_SHELL, "panel-client", panel, NULL);
}

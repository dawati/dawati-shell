/*
 * Carrick - a connection panel for the MeeGo Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Joshua Lock <josh@linux.intel.com>
 *              Jussi Kukkonen <jku@linux.intel.com>
 */

#include "carrick-pane.h"

#include <config.h>
#include <mx-gtk/mx-gtk.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include <dbus/dbus-glib.h>

#include "connman-marshal.h"
#include "connman-manager-bindings.h"
#include "connman-technology-bindings.h"

#include "carrick-list.h"
#include "carrick-service-item.h"
#include "carrick-icon-factory.h"
#include "carrick-notification-manager.h"
#include "carrick-network-model.h"
#include "carrick-shell.h"
#include "carrick-util.h"
#include "mux-banner.h"

G_DEFINE_TYPE (CarrickPane, carrick_pane, GTK_TYPE_HBOX)

#define PANE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_PANE, CarrickPanePrivate))

struct _CarrickPanePrivate
{
  GtkWidget *wifi_switch;
  GtkWidget *wifi_box;
  GtkWidget *wifi_sep;
  GtkWidget *ethernet_switch;
  GtkWidget *ethernet_box;
  GtkWidget *ethernet_sep;
  GtkWidget *threeg_switch;
  GtkWidget *threeg_box;
  GtkWidget *threeg_sep;
  GtkWidget *wimax_switch;
  GtkWidget *wimax_box;
  GtkWidget *wimax_sep;
  GtkWidget *bluetooth_switch;
  GtkWidget *bluetooth_box;
  GtkWidget *offline_mode_switch;
  GtkWidget *offline_mode_box;
  GtkWidget *service_list;
  GtkWidget *new_conn_button;
  GtkWidget *vpn_box;
  GtkWidget *vpn_combo;

  CarrickIconFactory *icon_factory;
  time_t   last_scan;
  gboolean offline_mode;
  gboolean have_daemon;
  gboolean have_wifi;
  gboolean have_ethernet;
  gboolean have_threeg;
  gboolean have_wimax;
  gboolean have_bluetooth;
  gboolean wifi_enabled;
  gboolean ethernet_enabled;
  gboolean threeg_enabled;
  gboolean wimax_enabled;
  gboolean bluetooth_enabled;

  GConfClient *gconf;
  DBusGProxy *manager;

  /* Technology proxies are needed to get State:Blocked
   * -- this means a hw or sw rfkill switch */
  DBusGProxy *wifi_tech;
  DBusGProxy *wimax_tech;
  DBusGProxy *bluetooth_tech;
  DBusGProxy *threeg_tech;
  DBusGProxy *ethernet_tech;

  CarrickNotificationManager *notes;
};

enum {
  PROP_0,
  PROP_ICON_FACTORY,
  PROP_NOTIFICATIONS
};

enum
{
  CONNECTION_CHANGED, /* FIXME rename to ACTIVE_CONNECTION_CHANGED? */
  LAST_SIGNAL
};

static guint _signals[LAST_SIGNAL] = { 0, };

#define AUTH_DIALOG LIBEXECDIR"/nm-openconnect-auth-dialog"

enum {
  COLUMN_VPN_NAME,
  COLUMN_VPN_UUID,
  N_VPN_COLUMNS
};


static gboolean _wifi_switch_callback (MxGtkLightSwitch *wifi_switch, gboolean new_state, CarrickPane *pane);
static gboolean _ethernet_switch_callback (MxGtkLightSwitch *wifi_switch, gboolean new_state, CarrickPane *pane);
static gboolean _wimax_switch_callback (MxGtkLightSwitch *wifi_switch, gboolean new_state, CarrickPane *pane);
static gboolean _threeg_switch_callback (MxGtkLightSwitch *wifi_switch, gboolean new_state, CarrickPane *pane);
static gboolean _bluetooth_switch_callback (MxGtkLightSwitch *wifi_switch, gboolean new_state, CarrickPane *pane);
static gboolean _offline_mode_switch_callback (MxGtkLightSwitch *wifi_switch, gboolean new_state, CarrickPane *pane);

static void
pane_manager_changed_cb (DBusGProxy  *proxy, const gchar *property,
                         GValue      *value, gpointer     user_data);

static void
pane_free_g_value (GValue *val)
{
  g_value_unset (val);
  g_slice_free (GValue, val);
}

static gboolean
_focus_callback (GtkWidget       *widget,
                 GtkDirectionType arg1,
                 gpointer         user_data)
{
  /*
   * Work around for bug #4319:
   * Stop propogating focus events to
   * contained widgets so that we do
   * not put items in the carrick-list
   * in a 'SELECTED' state
   */
  return TRUE;
}

static void
carrick_pane_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  CarrickPane        *pane = CARRICK_PANE (object);
  CarrickPanePrivate *priv = pane->priv;

  switch (property_id)
    {
    case PROP_ICON_FACTORY:
      g_value_set_object (value, priv->icon_factory);
      break;

    case PROP_NOTIFICATIONS:
      g_value_set_object (value, priv->notes);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_pane_set_icon_factory (CarrickPane        *pane,
                               CarrickIconFactory *icon_factory)
{
  CarrickPanePrivate *priv = pane->priv;

  priv->icon_factory = icon_factory;
  carrick_list_set_icon_factory (CARRICK_LIST (priv->service_list),
                                 icon_factory);
}

static void
carrick_pane_set_notifications (CarrickPane                *pane,
                                CarrickNotificationManager *notes)
{
  CarrickPanePrivate *priv = pane->priv;

  priv->notes = notes;
  carrick_list_set_notification_manager (CARRICK_LIST (priv->service_list),
                                         notes);
}

static void
carrick_pane_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CarrickPane *pane = CARRICK_PANE (object);

  switch (property_id)
    {
    case PROP_ICON_FACTORY:
      carrick_pane_set_icon_factory (
        pane,
        CARRICK_ICON_FACTORY (g_value_get_object (value)));
      break;

    case PROP_NOTIFICATIONS:
      carrick_pane_set_notifications (
        pane,
        CARRICK_NOTIFICATION_MANAGER (g_value_get_object (value)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_pane_dispose (GObject *object)
{
  CarrickPane *self = CARRICK_PANE (object);
  CarrickPanePrivate *priv = self->priv;

  if (priv->manager)
    {
      dbus_g_proxy_disconnect_signal (priv->manager,
                                      "PropertyChanged",
                                      G_CALLBACK (pane_manager_changed_cb),
                                      self);
      g_object_unref (priv->manager);
      priv->manager = NULL;
    }

  if (priv->gconf) {
    g_object_unref (priv->gconf);
    priv->gconf = NULL;
  }

  G_OBJECT_CLASS (carrick_pane_parent_class)->dispose (object);
}

static void
carrick_pane_class_init (CarrickPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;

  g_type_class_add_private (klass, sizeof (CarrickPanePrivate));

  object_class->get_property = carrick_pane_get_property;
  object_class->set_property = carrick_pane_set_property;
  object_class->dispose = carrick_pane_dispose;

  pspec = g_param_spec_object ("icon-factory",
                               "Icon factory",
                               "Icon factory to use",
                               CARRICK_TYPE_ICON_FACTORY,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_ICON_FACTORY,
                                   pspec);

  pspec = g_param_spec_object ("notification-manager",
                               "CarrickNotificationManager",
                               "Notification manager to use",
                               CARRICK_TYPE_NOTIFICATION_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_NOTIFICATIONS,
                                   pspec);

  _signals[CONNECTION_CHANGED] =
      g_signal_new ("connection-changed",
                    CARRICK_TYPE_PANE,
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET(CarrickPaneClass, connection_changed),
                    NULL,
                    NULL,
                    connman_marshal_VOID__STRING_STRING_STRING_UINT,
                    G_TYPE_NONE,
                    4,
                    G_TYPE_STRING,
                    G_TYPE_STRING,
                    G_TYPE_STRING,
                    G_TYPE_UINT);
}

static void
connect_service_notify_cb (DBusGProxy *proxy,
                           gchar      *service,
                           GError     *error,
                           gpointer    user_data)
{
  if (error)
    {
      g_debug ("Error when ending call: %s",
               error->message);
      g_error_free (error);
    }
}

/*
 * Generic call_notify function for async d-bus calls with no OUT parameters
 */
static void
dbus_proxy_notify_cb (DBusGProxy *proxy,
                      GError     *error,
                      gpointer    user_data)
{
  if (error)
    {
      g_debug ("Error when ending call: %s",
               error->message);
      g_error_free (error);
    }
}

static void
wifi_switch_notify_cb (DBusGProxy *proxy,
                       GError     *error,
                       gpointer    user_data)
{
  CarrickPane *pane = CARRICK_PANE (user_data);
  CarrickPanePrivate *priv = pane->priv;

  if (error)
    {
      g_debug ("Error when ending call: %s",
               error->message);
      g_error_free (error);

      mx_gtk_light_switch_set_active
        (MX_GTK_LIGHT_SWITCH (priv->wifi_switch),
         priv->wifi_enabled);
    }

  g_signal_handlers_unblock_by_func (priv->wifi_switch,
                                     _wifi_switch_callback,
                                     pane);
}

static gboolean
_wifi_switch_callback (MxGtkLightSwitch *wifi_switch,
                       gboolean            new_state,
                       CarrickPane        *pane)
{
  CarrickPanePrivate *priv = pane->priv;

  g_signal_handlers_block_by_func (wifi_switch,
                                   _wifi_switch_callback,
                                   pane);

  if (new_state)
    {
      carrick_notification_manager_queue_event (priv->notes,
                                                "wifi",
                                                "ready",
                                                "all");
      net_connman_Manager_enable_technology_async (priv->manager,
                                                   "wifi",
                                                   wifi_switch_notify_cb,
                                                   pane);
    }
  else
    {
      carrick_notification_manager_queue_event (priv->notes,
                                                "wifi",
                                                "idle",
                                                "all");
      net_connman_Manager_disable_technology_async (priv->manager,
                                                    "wifi",
                                                    wifi_switch_notify_cb,
                                                    pane);
    }

  return TRUE;
}

static void
ethernet_switch_notify_cb (DBusGProxy *proxy,
                           GError     *error,
                           gpointer    user_data)
{
  CarrickPane *pane = CARRICK_PANE (user_data);
  CarrickPanePrivate *priv = pane->priv;

  if (error)
    {
      g_debug ("Error when ending call: %s",
               error->message);
      g_error_free (error);

      mx_gtk_light_switch_set_active
        (MX_GTK_LIGHT_SWITCH (priv->ethernet_switch),
         priv->ethernet_enabled);
    }

  g_signal_handlers_unblock_by_func (priv->ethernet_switch,
                                     _ethernet_switch_callback,
                                     pane);
}

static gboolean
_ethernet_switch_callback (MxGtkLightSwitch *ethernet_switch,
                           gboolean            new_state,
                           CarrickPane        *pane)
{
  CarrickPanePrivate *priv = pane->priv;

  g_signal_handlers_block_by_func (priv->ethernet_switch,
                                   _ethernet_switch_callback,
                                   pane);

  if (new_state)
    {
      carrick_notification_manager_queue_event (priv->notes,
                                                "ethernet",
                                                "ready",
                                                "all");
      net_connman_Manager_enable_technology_async (priv->manager,
                                                   "ethernet",
                                                   ethernet_switch_notify_cb,
                                                   pane);
    }
  else
    {
      carrick_notification_manager_queue_event (priv->notes,
                                                "ethernet",
                                                "idle",
                                                "all");
      net_connman_Manager_disable_technology_async (priv->manager,
                                                    "ethernet",
                                                    ethernet_switch_notify_cb,
                                                    pane);
    }

  return TRUE;
}

static void
threeg_switch_notify_cb (DBusGProxy *proxy,
                         GError     *error,
                         gpointer    user_data)
{
  CarrickPane *pane = CARRICK_PANE (user_data);
  CarrickPanePrivate *priv = pane->priv;

  if (error)
    {
      g_debug ("Error when ending call: %s",
               error->message);
      g_error_free (error);

      mx_gtk_light_switch_set_active
        (MX_GTK_LIGHT_SWITCH (priv->threeg_switch),
         priv->threeg_enabled);
    }

  g_signal_handlers_unblock_by_func (priv->threeg_switch,
                                     _threeg_switch_callback,
                                     pane);
}

static gboolean
_threeg_switch_callback (MxGtkLightSwitch *threeg_switch,
                         gboolean            new_state,
                         CarrickPane        *pane)
{
  CarrickPanePrivate *priv = pane->priv;

  g_signal_handlers_block_by_func (threeg_switch,
                                   _threeg_switch_callback,
                                   pane);

  if (new_state)
    {
      carrick_notification_manager_queue_event (priv->notes,
                                                "cellular",
                                                "ready",
                                                "all");
      net_connman_Manager_enable_technology_async (priv->manager,
                                                   "cellular",
                                                   threeg_switch_notify_cb,
                                                   pane);
    }
  else
    {
      carrick_notification_manager_queue_event (priv->notes,
                                                "cellular",
                                                "idle",
                                                "all");
      net_connman_Manager_disable_technology_async (priv->manager,
                                                    "cellular",
                                                    threeg_switch_notify_cb,
                                                    pane);
    }

  return TRUE;
}

static void
wimax_switch_notify_cb (DBusGProxy *proxy,
                        GError     *error,
                        gpointer    user_data)
{
  CarrickPane *pane = CARRICK_PANE (user_data);
  CarrickPanePrivate *priv = pane->priv;

  if (error)
    {
      g_debug ("Error when ending call: %s",
               error->message);
      g_error_free (error);

      mx_gtk_light_switch_set_active
        (MX_GTK_LIGHT_SWITCH (priv->wimax_switch),
         priv->wimax_enabled);
    }

  g_signal_handlers_unblock_by_func (priv->wimax_switch,
                                     _wimax_switch_callback,
                                     pane);
}

static gboolean
_wimax_switch_callback (MxGtkLightSwitch *wimax_switch,
                        gboolean            new_state,
                        CarrickPane        *pane)
{
  CarrickPanePrivate *priv = pane->priv;

  g_signal_handlers_block_by_func (wimax_switch,
                                   _wimax_switch_callback,
                                   pane);

  if (new_state)
    {
      carrick_notification_manager_queue_event (priv->notes,
                                                "wimax",
                                                "ready",
                                                "all");
      net_connman_Manager_enable_technology_async (priv->manager,
                                                   "wimax",
                                                   wimax_switch_notify_cb,
                                                   pane);
    }
  else
    {
      carrick_notification_manager_queue_event (priv->notes,
                                                "wimax",
                                                "idle",
                                                "all");
      net_connman_Manager_disable_technology_async (priv->manager,
                                                    "wimax",
                                                    wimax_switch_notify_cb,
                                                    pane);
    }

  return TRUE;
}

static void
bluetooth_switch_notify_cb (DBusGProxy *proxy,
                            GError     *error,
                            gpointer    user_data)
{
  CarrickPane *pane = CARRICK_PANE (user_data);
  CarrickPanePrivate *priv = pane->priv;

  if (error)
    {
      g_debug ("Error when ending call: %s",
               error->message);
      g_error_free (error);

      mx_gtk_light_switch_set_active
        (MX_GTK_LIGHT_SWITCH (priv->bluetooth_switch),
         priv->bluetooth_enabled);
    }

  g_signal_handlers_unblock_by_func (priv->bluetooth_switch,
                                     _bluetooth_switch_callback,
                                     pane);
}

static gboolean
_bluetooth_switch_callback (MxGtkLightSwitch *bluetooth_switch,
                            gboolean            new_state,
                            CarrickPane        *pane)
{
  CarrickPanePrivate *priv = pane->priv;

  g_signal_handlers_block_by_func (bluetooth_switch,
                                   _bluetooth_switch_callback,
                                   pane);

  if (new_state)
    {
      carrick_notification_manager_queue_event (priv->notes,
                                                "bluetooth",
                                                "ready",
                                                "all");
      net_connman_Manager_enable_technology_async (priv->manager,
                                                   "bluetooth",
                                                   bluetooth_switch_notify_cb,
                                                   pane);
    }
  else
    {
      carrick_notification_manager_queue_event (priv->notes,
                                                "bluetooth",
                                                "idle",
                                                "all");
      net_connman_Manager_disable_technology_async (priv->manager,
                                                    "bluetooth",
                                                    bluetooth_switch_notify_cb,
                                                    pane);
    }

  return TRUE;
}

static void
_secret_check_toggled (GtkToggleButton *toggle,
                       gpointer         user_data)
{
  GtkEntry *entry = GTK_ENTRY (user_data);
  gboolean  vis = gtk_toggle_button_get_active (toggle);

  gtk_entry_set_visibility (entry, vis);
}

typedef struct secret_data {
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *check;
} secret_data;

static void
_security_combo_changed_cb (GtkComboBox *combo, secret_data *data)
{
  gboolean security;

  security = (gtk_combo_box_get_active (combo) != 0);

  gtk_widget_set_sensitive (data->label, security);
  gtk_widget_set_sensitive (data->check, security);
  gtk_widget_set_sensitive (data->entry, security);
  if (!security)
    gtk_entry_set_text (GTK_ENTRY (data->entry), "");
}

static void
_new_connection_cb (GtkButton *button,
                    gpointer   user_data)
{
  CarrickPane        *self = CARRICK_PANE (user_data);
  CarrickPanePrivate *priv = self->priv;
  GtkWidget          *dialog;
  GtkWidget          *desc;
  GtkWidget          *ssid_entry, *ssid_label;
  GtkWidget          *security_combo, *security_label;
  GtkWidget          *secret_entry, *secret_label;
  GtkWidget          *secret_check;
  GtkWidget          *info_bar, *content, *info_label;
  GtkWidget          *table;
  const gchar        *network, *secret;
  gchar              *security;
  GtkWidget          *image;
  GHashTable         *method_props;
  GValue             *type_v, *mode_v, *ssid_v, *security_v, *pass_v;
  secret_data        *secretdata;

  dialog = gtk_dialog_new_with_buttons (_ ("New connection settings"),
                                        GTK_WINDOW (gtk_widget_get_toplevel (user_data)),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_REJECT,
                                        GTK_STOCK_CONNECT,
                                        GTK_RESPONSE_ACCEPT, NULL);

  carrick_shell_close_dialog_on_hide (GTK_DIALOG (dialog));

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog),
                                FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   GTK_RESPONSE_ACCEPT);
  gtk_window_set_icon_name (GTK_WINDOW (dialog),
                            GTK_STOCK_NETWORK);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                       6);

  table = gtk_table_new (5,
                         2,
                         FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table),
                              6);
  gtk_table_set_col_spacings (GTK_TABLE (table),
                              6);
  gtk_container_set_border_width (GTK_CONTAINER (table),
                                  6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
                     table);
  image = gtk_image_new_from_icon_name (GTK_STOCK_NETWORK,
                                        GTK_ICON_SIZE_DIALOG);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             image,
                             0, 1,
                             0, 1);

  desc = gtk_label_new (_ ("Enter the name of the network you want\nto "
                           "connect to and, where necessary, the\n"
                           "password and security type."));
  gtk_table_attach_defaults (GTK_TABLE (table),
                             desc,
                             1, 3,
                             0, 1);
  gtk_misc_set_alignment (GTK_MISC (desc),
                          0.0, 0.5);

  ssid_label = gtk_label_new (_ ("Network name"));
  gtk_misc_set_alignment (GTK_MISC (ssid_label),
                          0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             ssid_label,
                             1, 2, 1, 2);

  ssid_entry = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table),
                             ssid_entry,
                             2, 3, 1, 2);

  security_label = gtk_label_new (_ ("Network security"));
  gtk_misc_set_alignment (GTK_MISC (security_label),
                          0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             security_label,
                             1, 2,
                             2, 3);

  security_combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                             _ ("None"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                             "WEP");
  gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                             "WPA");
  gtk_combo_box_set_active (GTK_COMBO_BOX (security_combo),
                            0);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             security_combo,
                             2, 3,
                             2, 3);

  secret_label = gtk_label_new (_ ("Password"));
  gtk_widget_set_sensitive (secret_label, FALSE);
  gtk_misc_set_alignment (GTK_MISC (secret_label),
                          0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             secret_label,
                             1, 2,
                             3, 4);

  secret_entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (secret_entry),
                            FALSE);
  gtk_widget_set_sensitive (secret_entry, FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             secret_entry,
                             2, 3,
                             3, 4);

  secret_check = gtk_check_button_new_with_label (_ ("Show password"));
  gtk_widget_set_sensitive (secret_check, FALSE);
  g_signal_connect (secret_check,
                    "toggled",
                    G_CALLBACK (_secret_check_toggled),
                    (gpointer) secret_entry);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             secret_check,
                             1, 2,
                             4, 5);

  secretdata = g_new0 (secret_data, 1);
  secretdata->entry = secret_entry;
  secretdata->label = secret_label;
  secretdata->check = secret_check;
  g_signal_connect (security_combo, "changed",
                    G_CALLBACK (_security_combo_changed_cb), secretdata);

  info_bar = gtk_info_bar_new ();
  gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar), GTK_MESSAGE_WARNING);
  gtk_table_attach_defaults (GTK_TABLE (table), info_bar,
                             1, 3,
                             5, 6);
  gtk_widget_set_no_show_all (info_bar, TRUE);

  info_label = gtk_label_new ("");
  gtk_widget_set_size_request (info_label, 280, -1);
  gtk_label_set_line_wrap (GTK_LABEL (info_label), TRUE);
  content = gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar));
  gtk_container_add (GTK_CONTAINER (content), info_label);
  gtk_widget_show (info_label);

  gtk_widget_show_all (dialog);

  while (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      char *message;

      network = gtk_entry_get_text (GTK_ENTRY (ssid_entry));
      security = gtk_combo_box_get_active_text (GTK_COMBO_BOX (security_combo));
      secret = gtk_entry_get_text (GTK_ENTRY (secret_entry));

      if (network == NULL)
        continue;

      if (gtk_combo_box_get_active (GTK_COMBO_BOX (security_combo)) == 0)
        {
          /* The 0th item is the None entry. Because we have marked that string
           * for translation we should convert it to a form that ConnMan recognises
           * if it has been selected */
          g_free (security);
          security = g_strdup ("none");
        }
      else if (g_strcmp0 (security, "WPA") == 0)
        {
          g_free (security);
          security = g_strdup ("psk");
        }
      else
        {
          guint i;
          for (i = 0; security[i] != '\0'; i++)
            {
              security[i] = g_ascii_tolower (security[i]);
            }
        }

      gtk_widget_hide (info_bar);
      if (!util_validate_wlan_passphrase (security,
                                          secret,
                                          &message))
        {
          if (message)
            {
              gtk_label_set_text (GTK_LABEL (info_label), message);
              gtk_widget_show (info_bar);
              g_free (message);
              message = NULL;
            }
          gtk_editable_select_region (GTK_EDITABLE (secret_entry),
                                      0, -1);
          gtk_widget_grab_focus (secret_entry);
          continue;
        }

      carrick_notification_manager_queue_event (priv->notes,
                                                "wifi",
                                                "ready",
                                                network);
      /*
       * Call method to connect
       */
      method_props = g_hash_table_new_full (g_str_hash,
                                            g_str_equal,
                                            g_free,
                                            (GDestroyNotify) pane_free_g_value);
      type_v = g_slice_new0 (GValue);
      g_value_init (type_v, G_TYPE_STRING);
      g_value_set_string (type_v, g_strdup ("wifi"));
      g_hash_table_insert (method_props, g_strdup ("Type"), type_v);

      mode_v = g_slice_new0 (GValue);
      g_value_init (mode_v, G_TYPE_STRING);
      g_value_set_string (mode_v, g_strdup ("managed"));
      g_hash_table_insert (method_props, g_strdup ("Mode"), mode_v);

      ssid_v = g_slice_new0 (GValue);
      g_value_init (ssid_v, G_TYPE_STRING);
      g_value_set_string (ssid_v, network);
      g_hash_table_insert (method_props, g_strdup ("SSID"), ssid_v);

      security_v = g_slice_new0 (GValue);
      g_value_init (security_v, G_TYPE_STRING);
      if (security)
        g_value_set_string (security_v, security);
      else
        g_value_set_string (security_v, "none");
      g_hash_table_insert (method_props, g_strdup ("Security"), security_v);

      pass_v = g_slice_new0 (GValue);
      g_value_init (pass_v, G_TYPE_STRING);
      if (secret)
        g_value_set_string (pass_v, secret);
      else
        g_value_set_string (pass_v, "");
      g_hash_table_insert (method_props, g_strdup ("Passphrase"), pass_v);

      /* Connection methods do not return until there has been success or an error,
       * set the timeout nice and long before we make the call
       */
      dbus_g_proxy_set_default_timeout (priv->manager, 120000);
      net_connman_Manager_connect_service_async (priv->manager,
                                                 method_props,
                                                 connect_service_notify_cb,
                                                 self);
      dbus_g_proxy_set_default_timeout (priv->manager, -1);

      break;
    }

  /*
   * Now explicitely focus the panel (MB#7304)
   */
  carrick_shell_request_focus ();

  g_free (secretdata);
  gtk_widget_destroy (dialog);
}

static void
offline_switch_notify_cb (DBusGProxy *proxy,
                          GError     *error,
                          gpointer    user_data)
{
  CarrickPane *pane = CARRICK_PANE (user_data);
  CarrickPanePrivate *priv = pane->priv;

  if (error)
    {
      g_debug ("Error when ending call: %s",
               error->message);
      g_error_free (error);

      mx_gtk_light_switch_set_active
        (MX_GTK_LIGHT_SWITCH (priv->offline_mode_switch),
         priv->offline_mode);
    }

  g_signal_handlers_unblock_by_func (priv->offline_mode_switch,
                                     _offline_mode_switch_callback,
                                     pane);
}

static gboolean
_offline_mode_switch_callback (MxGtkLightSwitch *flight_switch,
                               gboolean            new_state,
                               CarrickPane        *pane)
{
  CarrickPanePrivate *priv = pane->priv;
  GValue *value;

  g_signal_handlers_block_by_func (priv->offline_mode_switch,
                                   _offline_mode_switch_callback,
                                   pane);

  /* FIXME: This is a band aid, needs a better fix */
  carrick_notification_manager_queue_event (priv->notes,
                                            "all",
                                            "idle",
                                            "all");

  value = g_slice_new0 (GValue);
  g_value_init (value, G_TYPE_BOOLEAN);
  g_value_set_boolean (value,
                       new_state);

  net_connman_Manager_set_property_async (priv->manager,
                                          "OfflineMode",
                                          value,
                                          offline_switch_notify_cb,
                                          pane);

  g_value_unset (value);
  g_slice_free (GValue,
                value);

  return TRUE;
}

static void
tech_property_changed_cb (DBusGProxy  *proxy,
                          const gchar *property,
                          GValue      *value,
                          gpointer     data)
{
  GtkWidget *tech_switch = GTK_WIDGET (data);

  if (g_strcmp0 (property, "State") == 0)
    {
      gboolean blocked;

      blocked = (g_strcmp0 (g_value_get_string (value), "blocked") == 0);
      gtk_widget_set_sensitive (tech_switch, !blocked);
    }
}

static void
tech_get_properties_cb (DBusGProxy *tech_proxy,
                        GHashTable *properties,
                        GError     *error,
                        gpointer    data)
{
  GtkWidget *tech_switch = GTK_WIDGET (data);
  GValue *value;

  if (error)
    {
      g_debug ("Error when ending Technology.GetProperties call: %s",
               error->message);
      g_error_free (error);
      return;
    }

  value = g_hash_table_lookup (properties, "State");
  if (value)
    {
      gboolean blocked;

      blocked = (g_strcmp0 (g_value_get_string (value), "blocked") == 0);
      gtk_widget_set_sensitive (tech_switch, !blocked);
    }
}

static DBusGProxy*
new_tech_proxy (const char *tech, GtkWidget *tech_switch)
{
  DBusGProxy *proxy;
  DBusGConnection *connection;
  char *path;

  /* default in case something fails when checking tech State */
  gtk_widget_set_sensitive (tech_switch, TRUE);

  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, NULL);
  if (!connection) 
    return NULL;

  path = g_strdup_printf ("/net/connman/technology/%s", tech);
  proxy = dbus_g_proxy_new_for_name (connection,
                                     CONNMAN_SERVICE,
                                     path,
                                     "net.connman.Technology");
  g_free (path);

  /* Listen for the D-Bus PropertyChanged signal */
  dbus_g_proxy_add_signal (proxy,
                           "PropertyChanged",
                           G_TYPE_STRING,
                           G_TYPE_VALUE,
                           G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy,
                               "PropertyChanged",
                               G_CALLBACK (tech_property_changed_cb),
                               tech_switch,
                               NULL);

  net_connman_Technology_get_properties_async (proxy,
                                               tech_get_properties_cb,
                                               tech_switch);
  
  return proxy;
}

static void
unref_tech_proxy (DBusGProxy *proxy, gpointer data)
{
  if (proxy)
    {
      dbus_g_proxy_disconnect_signal (proxy,
                                      "PropertyChanged",
                                      G_CALLBACK (tech_property_changed_cb),
                                      data);
      g_object_unref (proxy);
    }
}

static void
pane_update_property (const gchar *property,
                      GValue      *value,
                      gpointer     user_data)
{
  CarrickPane        *self = user_data;
  CarrickPanePrivate *priv = self->priv;

  if (g_str_equal (property, "OfflineMode"))
    {
      priv->offline_mode = g_value_get_boolean (value);

      /* disarm signal handler */
      g_signal_handlers_block_by_func (priv->offline_mode_switch,
                                       _offline_mode_switch_callback,
                                       user_data);

      mx_gtk_light_switch_set_active (MX_GTK_LIGHT_SWITCH
                                        (priv->offline_mode_switch),
                                        priv->offline_mode);

      /* arm signal handler */
      g_signal_handlers_unblock_by_func (priv->offline_mode_switch,
                                         _offline_mode_switch_callback,
                                         user_data);
    }
  /*else if (g_str_equal (property, "State"))
     {
     }*/
  else if (g_str_equal (property, "AvailableTechnologies"))
    {
      gchar **tech = g_value_get_boxed (value);
      gint    i;

      priv->have_wifi = FALSE;
      priv->have_ethernet = FALSE;
      priv->have_threeg = FALSE;
      priv->have_wimax = FALSE;
      priv->have_bluetooth = FALSE;

      unref_tech_proxy (priv->wifi_tech, priv->wifi_switch);
      priv->wifi_tech = NULL;
      unref_tech_proxy (priv->wimax_tech, priv->wimax_switch);
      priv->wimax_tech = NULL;
      unref_tech_proxy (priv->bluetooth_tech, priv->bluetooth_switch);
      priv->bluetooth_tech = NULL;
      unref_tech_proxy (priv->threeg_tech, priv->threeg_switch);
      priv->threeg_tech = NULL;
      unref_tech_proxy (priv->ethernet_tech, priv->ethernet_switch);
      priv->ethernet_tech = NULL;

      for (i = 0; i < g_strv_length (tech); i++)
        {
          const char *tech_name = (*(tech + i));

          if (g_str_equal ("wifi", tech_name))
            {
              priv->have_wifi = TRUE;
              priv->wifi_tech = new_tech_proxy (tech_name,
                                                priv->wifi_switch);
            }
          else if (g_str_equal ("wimax", tech_name))
            {
              priv->have_wimax = TRUE;
              priv->wimax_tech = new_tech_proxy (tech_name,
                                                 priv->wimax_switch);
            }
          else if (g_str_equal ("bluetooth", tech_name))
            {
              priv->have_bluetooth = TRUE;
              priv->bluetooth_tech = new_tech_proxy (tech_name,
                                                     priv->bluetooth_switch);
            }
          else if (g_str_equal ("cellular", tech_name))
            {
              priv->have_threeg = TRUE;
              priv->threeg_tech = new_tech_proxy (tech_name,
                                                  priv->threeg_switch);
            }
          else if (g_str_equal ("ethernet", tech_name))
            {
              priv->have_ethernet = TRUE;
              priv->ethernet_tech = new_tech_proxy (tech_name,
                                                    priv->ethernet_switch);
            }
        }

      gtk_widget_set_visible (priv->wifi_box,
                              priv->have_wifi);
      gtk_widget_set_visible (priv->wifi_sep,
                              priv->have_wifi &&
                              (priv->have_ethernet || priv->have_threeg ||
                               priv->have_wimax || priv->have_bluetooth));

      gtk_widget_set_visible (priv->ethernet_box,
                              priv->have_ethernet);
      gtk_widget_set_visible (priv->ethernet_sep,
                              priv->have_ethernet &&
                              (priv->have_threeg || priv->have_wimax ||
                               priv->have_bluetooth));

      gtk_widget_set_visible (priv->threeg_box,
                              priv->have_threeg);
      gtk_widget_set_visible (priv->threeg_sep,
                              priv->have_threeg &&
                              (priv->have_wimax || priv->have_bluetooth));

      gtk_widget_set_visible (priv->wimax_box,
                              priv->have_wimax);
      gtk_widget_set_visible (priv->wimax_sep,
                              priv->have_wimax && priv->have_bluetooth);

      gtk_widget_set_visible (priv->bluetooth_box,
                              priv->have_bluetooth);

    }
  else if (g_str_equal (property, "EnabledTechnologies"))
    {
      gchar **tech = g_value_get_boxed (value);
      gint    i;

      priv->wifi_enabled = FALSE;
      priv->ethernet_enabled = FALSE;
      priv->threeg_enabled = FALSE;
      priv->wimax_enabled = FALSE;
      priv->bluetooth_enabled = FALSE;

      for (i = 0; i < g_strv_length (tech); i++)
        {
          if (g_str_equal ("wifi", *(tech + i)))
            {
              priv->wifi_enabled = TRUE;
            }
          else if (g_str_equal ("wimax", *(tech + i)))
            {
              priv->wimax_enabled = TRUE;
            }
          else if (g_str_equal ("bluetooth", *(tech + i)))
            {
              priv->bluetooth_enabled = TRUE;
            }
          else if (g_str_equal ("cellular", *(tech + i)))
            {
              priv->threeg_enabled = TRUE;
            }
          else if (g_str_equal ("ethernet", *(tech + i)))
            {
              priv->ethernet_enabled = TRUE;
            }
        }

      /* disarm signal handlers */
      g_signal_handlers_block_by_func (priv->ethernet_switch,
                                       _ethernet_switch_callback,
                                       user_data);
      g_signal_handlers_block_by_func (priv->wifi_switch,
                                       _wifi_switch_callback,
                                       user_data);
      g_signal_handlers_block_by_func (priv->threeg_switch,
                                       _threeg_switch_callback,
                                       user_data);
      g_signal_handlers_block_by_func (priv->wimax_switch,
                                       _wimax_switch_callback,
                                       user_data);
      g_signal_handlers_block_by_func (priv->bluetooth_switch,
                                       _bluetooth_switch_callback,
                                       user_data);

      mx_gtk_light_switch_set_active (MX_GTK_LIGHT_SWITCH
                                        (priv->ethernet_switch),
                                        priv->ethernet_enabled);
      mx_gtk_light_switch_set_active (MX_GTK_LIGHT_SWITCH
                                        (priv->wifi_switch),
                                        priv->wifi_enabled);
      mx_gtk_light_switch_set_active (MX_GTK_LIGHT_SWITCH
                                        (priv->threeg_switch),
                                        priv->threeg_enabled);
      mx_gtk_light_switch_set_active (MX_GTK_LIGHT_SWITCH
                                        (priv->wimax_switch),
                                        priv->wimax_enabled);
      mx_gtk_light_switch_set_active (MX_GTK_LIGHT_SWITCH
                                        (priv->bluetooth_switch),
                                        priv->bluetooth_enabled);

      /* arm signal handlers */
      g_signal_handlers_unblock_by_func (priv->ethernet_switch,
                                         _ethernet_switch_callback,
                                         user_data);
      g_signal_handlers_unblock_by_func (priv->wifi_switch,
                                         _wifi_switch_callback,
                                         user_data);
      g_signal_handlers_unblock_by_func (priv->threeg_switch,
                                         _threeg_switch_callback,
                                         user_data);
      g_signal_handlers_unblock_by_func (priv->wimax_switch,
                                         _wimax_switch_callback,
                                         user_data);
      g_signal_handlers_unblock_by_func (priv->bluetooth_switch,
                                         _bluetooth_switch_callback,
                                         user_data);

      /* only enable "Add new connection" if wifi is enabled */
      gtk_widget_set_sensitive (priv->new_conn_button,
                                priv->wifi_enabled);
    }
}

static void
pane_have_daemon (CarrickPane *pane,
                  gboolean     have_daemon)
{
  CarrickPanePrivate *priv = pane->priv;

  gtk_widget_set_sensitive (priv->service_list, have_daemon);
  gtk_widget_set_sensitive (priv->new_conn_button, have_daemon);
  gtk_widget_set_visible (priv->offline_mode_box, have_daemon);

  if (!have_daemon)
    {
      gtk_widget_set_visible (priv->wifi_box, FALSE);
      gtk_widget_set_visible (priv->wifi_sep, FALSE);
      gtk_widget_set_visible (priv->ethernet_box, FALSE);
      gtk_widget_set_visible (priv->ethernet_sep, FALSE);
      gtk_widget_set_visible (priv->threeg_box, FALSE);
      gtk_widget_set_visible (priv->threeg_sep, FALSE);
      gtk_widget_set_visible (priv->wimax_box, FALSE);
      gtk_widget_set_visible (priv->wimax_sep, FALSE);
      gtk_widget_set_visible (priv->bluetooth_box, FALSE);
    }
}

static void
pane_manager_changed_cb (DBusGProxy  *proxy,
                         const gchar *property,
                         GValue      *value,
                         gpointer     user_data)
{
  CarrickPane *self = user_data;

  pane_update_property (property, value, self);
}

static void
pane_manager_get_properties_cb (DBusGProxy *manager,
                                GHashTable *properties,
                                GError     *error,
                                gpointer    user_data)
{
  CarrickPane *self = user_data;

  if (error)
    {
      g_debug ("Error when ending GetProperties call: %s",
               error->message);
      g_error_free (error);
      pane_have_daemon (self,
                        FALSE);
    }
  else
    {
      g_hash_table_foreach (properties,
                            (GHFunc) pane_update_property,
                            self);
      g_hash_table_unref (properties);
    }
}

static void
pane_update_vpn_ui (CarrickPane *self,
                    const char *connection_type,
                    const char *connection_state)
{
  if (g_strcmp0 (connection_type, "vpn") == 0 &&
      (g_strcmp0 (connection_state, "online") == 0 ||
       g_strcmp0 (connection_state, "ready") == 0 ||
       g_strcmp0 (connection_state, "configuration") == 0 ||
       g_strcmp0 (connection_state, "association") == 0))
    {
      /* there is a VPN connection already */
      gtk_widget_set_sensitive (self->priv->vpn_box, FALSE);
    }
  else if (g_strcmp0 (connection_type, "vpn") != 0 &&
           (g_strcmp0 (connection_state, "online") == 0 ||
            g_strcmp0 (connection_state, "ready") == 0))
    {
      /* there is a network connection */
      gtk_widget_set_sensitive (self->priv->vpn_box, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (self->priv->vpn_box, FALSE);
    }
}

static void
model_emit_change (GtkTreeModel *tree_model,
                   GtkTreeIter  *iter,
                   CarrickPane  *self)
{
  CarrickPanePrivate *priv = self->priv;
  char *connection_type = NULL;
  char *connection_name = NULL;
  char *connection_state = NULL;
  guint strength = 0;

  gtk_tree_model_get (tree_model, iter,
                      CARRICK_COLUMN_TYPE, &connection_type,
                      CARRICK_COLUMN_NAME, &connection_name,
                      CARRICK_COLUMN_STRENGTH, &strength,
                      CARRICK_COLUMN_STATE, &connection_state,
                      -1);

  if (!connection_type)
    connection_type = g_strdup ("");
  if (!connection_name)
    connection_name = g_strdup ("");
  if (!connection_state)
    connection_state = g_strdup ("");

  g_signal_emit (self, _signals[CONNECTION_CHANGED], 0,
                 connection_type,
                 connection_name,
                 connection_state,
                 strength);

  /* We *may* need to provide a notification */
  carrick_notification_manager_notify_event (priv->notes,
                                             connection_type,
                                             connection_state,
                                             connection_name,
                                             strength);

  pane_update_vpn_ui (self, connection_type, connection_state);
}

static void
model_emit_empty (CarrickPane *self)
{
  g_signal_emit (self, _signals[CONNECTION_CHANGED], 0,
                 "",
                 "",
                 "idle",
                 0);
}

static void
model_row_deleted_cb (GtkTreeModel  *tree_model,
                      GtkTreePath   *path,
                      CarrickPane   *self)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (tree_model,
                                     &iter))
    {
      model_emit_change (tree_model,
                         &iter,
                         self);
    }
  else
    {
      model_emit_empty (self);
    }
}

static void
model_row_changed_cb (GtkTreeModel  *tree_model,
                      GtkTreePath   *path,
                      GtkTreeIter   *iter,
                      CarrickPane   *self)
{
  guint index;

  gtk_tree_model_get (tree_model, iter,
                      CARRICK_COLUMN_INDEX, &index,
                      -1);
  if (index == 0) {
    model_emit_change (tree_model,
                       iter,
                       self);
  }
}

static void
pane_name_owner_changed (DBusGProxy  *proxy,
                         const gchar *name,
                         const gchar *prev_owner,
                         const gchar *new_owner,
                         CarrickPane *self)
{
  if (!g_str_equal (name, CONNMAN_SERVICE))
    {
      return;
    }

  if (new_owner[0] == '\0')
    {
      pane_have_daemon (self, FALSE);
      return;
    }

  pane_have_daemon (self, TRUE);
}

static GtkWidget*
get_technology_box (const char *name,
                    GtkWidget **tech_switch)
{
  GtkWidget *tech_box, *box;
  GtkWidget *switch_label;
  char *title;

  tech_box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (tech_box);
  box = gtk_hbox_new (FALSE, 12);
  gtk_widget_show (box);
  *tech_switch = mx_gtk_light_switch_new ();
  gtk_widget_show (*tech_switch);

  switch_label = gtk_label_new (NULL);
  title = g_strdup_printf ("<b>%s</b>", name);
  gtk_label_set_markup (GTK_LABEL (switch_label), title);
  g_free (title);
  gtk_misc_set_padding (GTK_MISC (switch_label), 8, 0);
  gtk_widget_show (switch_label);


  gtk_box_pack_start (GTK_BOX (tech_box), box,
                      TRUE, TRUE, 12);
  gtk_box_pack_start (GTK_BOX (box), switch_label,
                      FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (box), *tech_switch,
                    FALSE, FALSE, 0);

  return tech_box;
}

static void
button_size_request_cb (GtkWidget      *button,
                        GtkRequisition *requisition,
                        gpointer        user_data)
{
  requisition->width = MAX (requisition->width, CARRICK_MIN_BUTTON_WIDTH);
}

static void
connect_provider_cb (DBusGProxy *proxy,
                     gchar      *service,
                     GError     *error,
                     gpointer    data)
{
  if (error)
    {
      g_debug ("Error when ending call: %s", error->message);
      g_error_free (error);
      return;
    }

  carrick_shell_show ();
}

static void
carrick_pane_connect_vpn (CarrickPane *self,
                          const char *name,
                          const char *gw,
                          const char *cookie,
                          const char *server_cert)
{
  GHashTable *props;
  GValue *val;

  props = g_hash_table_new_full (g_str_hash,
                                 g_str_equal,
                                 g_free,
                                 (GDestroyNotify) pane_free_g_value);

  val = g_slice_new0 (GValue);
  g_value_init (val, G_TYPE_STRING);
  g_value_set_string (val, g_strdup ("openconnect"));
  g_hash_table_insert (props, g_strdup ("Type"), val);

  val = g_slice_new0 (GValue);
  g_value_init (val, G_TYPE_STRING);
  g_value_set_string (val, g_strdup (name));
  g_hash_table_insert (props, g_strdup ("Name"), val);

  val = g_slice_new0 (GValue);
  g_value_init (val, G_TYPE_STRING);
  g_value_set_string (val, g_strdup (gw));
  g_hash_table_insert (props, g_strdup ("Host"), val);

  val = g_slice_new0 (GValue);
  g_value_init (val, G_TYPE_STRING);
  g_value_set_string (val, g_strdup (cookie));
  g_hash_table_insert (props, g_strdup ("OpenConnect.Cookie"), val);

  if (server_cert)
    {
      val = g_slice_new0 (GValue);
      g_value_init (val, G_TYPE_STRING);
      g_value_set_string (val, g_strdup (server_cert));
      g_hash_table_insert (props, g_strdup ("OpenConnect.ServerCert"), val);
    }

  val = g_slice_new0 (GValue);
  g_value_init (val, G_TYPE_STRING);
  /* FIXME: VPN.Domain should be the domain to use for route splitting
   * but currently connman (0.61) does not do that, so we're being lazy
   * as well. */
  g_value_set_string (val, g_strdup (""));
  g_hash_table_insert (props, g_strdup ("VPN.Domain"), val);

  carrick_notification_manager_queue_event (self->priv->notes,
                                            "vpn",
                                            "ready",
                                            name);

  carrick_notification_manager_queue_event (self->priv->notes,
                                            "vpn",
                                            "ready",
                                            name);

  /* Connection methods do not return until there has been success or an error,
   * set the timeout nice and long before we make the call */
  dbus_g_proxy_set_default_timeout (self->priv->manager, 120000);
  net_connman_Manager_connect_provider_async (self->priv->manager,
                                              props,
                                              connect_provider_cb,
                                              self);
  dbus_g_proxy_set_default_timeout (self->priv->manager, -1);
}

typedef struct {
  CarrickPane *self;
  GIOChannel *io_out;
  char *name;
} auth_dialog_data;

static void
auth_dialog_exit_cb (GPid pid, int status, auth_dialog_data *data)
{
  char *line;
  char *gw = NULL, *cookie = NULL, *gw_cert = NULL;
  char *gw_term, *cookie_term;
  char *gw_cert_term = NULL;

  g_spawn_close_pid (pid);

  if (status != 0)
    {
      g_io_channel_unref (data->io_out);
      g_free (data->name);
      g_free (data);
      g_warning ("OpenConnect authentication failed\n");
      return;
    }

  g_io_channel_read_line (data->io_out, &line, NULL, NULL, NULL);
  while (line)
    {
      if (g_strcmp0 (line, "gateway\n") == 0)
        g_io_channel_read_line (data->io_out, &gw, NULL, NULL, NULL);
      else if (g_strcmp0 (line, "cookie\n") == 0)
        g_io_channel_read_line (data->io_out, &cookie, NULL, NULL, NULL);
      else if (g_strcmp0 (line, "gwcert\n") == 0)
        g_io_channel_read_line (data->io_out, &gw_cert, NULL, NULL, NULL);

      g_free (line);
      g_io_channel_read_line (data->io_out, &line, NULL, NULL, NULL);
    }
  g_io_channel_unref (data->io_out);

  if (!gw || !cookie)
    {
      g_free (data->name);
      g_free (data);
      g_free (gw);
      g_free (cookie);
      g_free (gw_cert);
      g_warning ("OpenConnect authentication did not return gateway and cookie\n");
      return;
    }

  /* remove linefeeds */
  gw_term = g_strndup (gw, strlen (gw) - 1);
  g_free (gw);

  cookie_term = g_strndup (cookie, strlen (cookie) - 1);
  g_free (cookie);

  if (gw_cert)
    {
      gw_cert_term = g_strndup (gw_cert, strlen (gw_cert) - 1);
      g_free (gw_cert);
    }

  carrick_pane_connect_vpn (data->self, data->name,
                            gw_term, cookie_term, gw_cert_term); 
  g_free (data->name);
  g_free (data);
  g_free (gw_term);
  g_free (cookie_term);
  g_free (gw_cert_term);
}

static void
vpn_connect_clicked (GtkButton *btn, CarrickPane *self)
{
  CarrickPanePrivate *priv;
  GtkTreeModel *model;
  GtkTreeIter iter;
  char *uuid, *name;
  GPid pid;
  int out;
  auth_dialog_data *data;
  GError *error = NULL;

  priv = self->priv;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->vpn_combo),
                                      &iter))
    {
      return;
    }

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->vpn_combo));
  gtk_tree_model_get (model, &iter,
                      COLUMN_VPN_NAME, &name,
                      COLUMN_VPN_UUID, &uuid,
                      -1);

  char *argv[] = {
    AUTH_DIALOG,
    "-u", uuid,
    "-n", name,
    "-s", "org.freedesktop.NetworkManager.openconnect",
    NULL
  };

  if (!g_spawn_async_with_pipes (NULL, argv, NULL,
                                 G_SPAWN_DO_NOT_REAP_CHILD,
                                 NULL, NULL,
                                 &pid, NULL, &out, NULL, &error))
    {
      g_warning ("Unable to spawn OpenConnect authentication dialog: %s",
                 error->message);
      g_error_free(error);
      return;
    }

  data = g_new0 (auth_dialog_data, 1);
  data->io_out = g_io_channel_unix_new (out);
  data->name = g_strdup (name);
  data->self = self;
  g_child_watch_add (pid, (GChildWatchFunc)auth_dialog_exit_cb, data);
}

static void
carrick_pane_load_vpn_config (CarrickPane *self)
{
  CarrickPanePrivate *priv;
  GSList *connections, *this;
  GtkListStore *store;
  GtkTreeIter iter;
  gboolean have_vpn = FALSE;

  priv = self->priv;

  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->vpn_combo)));
  gtk_list_store_clear (store);

  connections = gconf_client_all_dirs (priv->gconf,
                                       "/system/networking/connections",
                                       NULL);
  for (this = connections; this; this = this->next)
    {
      char *key, *val, *uuid, *id;

      /* make sure this is a vpn and uses openconnect */
      key = g_strdup_printf ("%s/connection/type", (char*)this->data);
      val = gconf_client_get_string (priv->gconf, key, NULL);
      g_free (key);

      if (g_strcmp0	(val, "vpn") != 0)
        {
          g_free (val);
          continue;
        }
      g_free (val);

      key = g_strdup_printf ("%s/vpn/service-type", (char*)this->data);
      val = gconf_client_get_string (priv->gconf, key, NULL);
      g_free (key);

      if (g_strcmp0	(val, "org.freedesktop.NetworkManager.openconnect") != 0)
        {
          g_free (val);
          continue;
        }
      g_free (val);

      key = g_strdup_printf("%s/connection/uuid", (char*)this->data);
      uuid = gconf_client_get_string (priv->gconf, key, NULL);
      g_free (key);

      key = g_strdup_printf("%s/connection/id", (char*)this->data);
      id = gconf_client_get_string (priv->gconf, key, NULL);
      g_free (key);

      /* TODO add vpn to to combo-model */
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLUMN_VPN_NAME, id,
                          COLUMN_VPN_UUID, uuid,
                          -1);
      g_free (uuid);
      g_free (id);
 
      if (!have_vpn)
        {
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->vpn_combo),
                                        &iter);
          have_vpn = TRUE;
        }
    }

  gtk_widget_set_visible (priv->vpn_box, have_vpn);

  g_slist_foreach (connections, (GFunc)g_free, NULL);
  g_slist_free (connections);
}

static void
vpn_config_changed_cb (GConfClient *gconf,
                       guint cnxn_id,
                       GConfEntry *entry,
                       gpointer user_data)
{
  carrick_pane_load_vpn_config (CARRICK_PANE (user_data));
}

static void
init_gconf (CarrickPane *self)
{
  self->priv->gconf = gconf_client_get_default ();
  gconf_client_add_dir (self->priv->gconf,
                        "/system/networking/connections",
                        GCONF_CLIENT_PRELOAD_NONE, NULL);
  gconf_client_notify_add (self->priv->gconf,
                           "/system/networking/connections",
                           vpn_config_changed_cb, self,
                           NULL, NULL);
  carrick_pane_load_vpn_config (self);
}

static void
carrick_pane_init (CarrickPane *self)
{
  CarrickPanePrivate *priv;
  GtkSizeGroup       *banner_group;
  GtkWidget          *settings_frame;
  GtkWidget          *net_list_frame;
  GtkWidget          *column;
  GtkWidget          *align;
  GtkWidget          *box;
  GtkWidget          *sep;
  GtkWidget          *offline_mode_label;
  GtkWidget          *banner;
  GError             *error = NULL;
  DBusGConnection    *connection;
  DBusGProxy         *bus_proxy;
  GtkWidget          *vpn_connect_button;
  GtkTreeModel       *model;
  GtkListStore       *vpn_store;
  GtkCellRenderer    *renderer;

  priv = self->priv = PANE_PRIVATE (self);

  priv->icon_factory = NULL;
  priv->manager = NULL;
  priv->last_scan = time (NULL);
  priv->have_daemon = FALSE;
  priv->have_wifi = FALSE;
  priv->have_ethernet = FALSE;
  priv->have_threeg = FALSE;
  priv->have_wimax = FALSE;
  priv->have_bluetooth = FALSE;
  priv->wifi_enabled = FALSE;
  priv->ethernet_enabled = FALSE;
  priv->threeg_enabled = FALSE;
  priv->wimax_enabled = FALSE;
  priv->bluetooth_enabled = FALSE;

  dbus_g_object_register_marshaller (connman_marshal_VOID__STRING_BOXED,
                                     /* return */
                                     G_TYPE_NONE,
                                     /* args */
                                     G_TYPE_STRING,
                                     G_TYPE_VALUE,
                                     /* eom */
                                     G_TYPE_INVALID);

  /* Get D-Bus connection and create proxy */
  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
  if (error)
    {
      g_debug ("Error connecting to bus: %s",
               error->message);
      g_error_free (error);
      /* FIXME: do something */
    }
  priv->manager = dbus_g_proxy_new_for_name (connection,
                                             CONNMAN_SERVICE,
                                             CONNMAN_MANAGER_PATH,
                                             CONNMAN_MANAGER_INTERFACE);

  model = carrick_network_model_new ();
  g_signal_connect (model,
                    "row-changed",
                    G_CALLBACK (model_row_changed_cb),
                    self);
  g_signal_connect (model,
                    "row-deleted",
                    G_CALLBACK (model_row_deleted_cb),
                    self);
  g_signal_connect (model,
                    "row-inserted",
                    G_CALLBACK (model_row_changed_cb),
                    self);

  /* Listen for the D-Bus PropertyChanged signal */
  dbus_g_proxy_add_signal (priv->manager,
                           "PropertyChanged",
                           G_TYPE_STRING,
                           G_TYPE_VALUE,
                           G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (priv->manager,
                               "PropertyChanged",
                               G_CALLBACK (pane_manager_changed_cb),
                               self,
                               NULL);

  /* Listen for NameOwnerChanged */
  bus_proxy = dbus_g_proxy_new_for_name (connection,
                                         DBUS_SERVICE_DBUS,
                                         DBUS_PATH_DBUS,
                                         DBUS_INTERFACE_DBUS);
  dbus_g_proxy_add_signal (bus_proxy,
                           "NameOwnerChanged",
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (bus_proxy,
                               "NameOwnerChanged",
                               G_CALLBACK (pane_name_owner_changed),
                               self,
                               NULL);

  net_connman_Manager_get_properties_async (priv->manager,
                                            pane_manager_get_properties_cb,
                                            self);

  banner_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

  /* Set box (self) up */
  gtk_box_set_spacing (GTK_BOX (self), 4);
  gtk_container_set_border_width (GTK_CONTAINER (self), 4);

  /*
   * Left column
   */
  settings_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (settings_frame), GTK_SHADOW_OUT);
  gtk_widget_show (settings_frame);

  column = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (column);

  banner = mux_banner_new (_("Settings"));
  gtk_size_group_add_widget (banner_group, banner);
  gtk_widget_show (banner);
  gtk_box_pack_start (GTK_BOX (column), banner, FALSE, FALSE, 0);

  align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_widget_show (align);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 8, 8);
  gtk_box_pack_start (GTK_BOX (column), align,
                      FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (box);
  gtk_container_add (GTK_CONTAINER (align), box);

  /* Switches */

  priv->wifi_box = get_technology_box (_ ("WiFi"),
                                       &priv->wifi_switch);
  gtk_box_pack_start (GTK_BOX (box), priv->wifi_box,
                      FALSE, FALSE, 0);
  g_signal_connect (MX_GTK_LIGHT_SWITCH (priv->wifi_switch),
                    "switch-flipped",
                    G_CALLBACK (_wifi_switch_callback),
                    self);

  priv->wifi_sep = gtk_hseparator_new ();
  gtk_widget_show (priv->wifi_sep);
  gtk_box_pack_start (GTK_BOX (box), priv->wifi_sep,
                      FALSE, FALSE, 0);

  priv->ethernet_box = get_technology_box (_ ("Wired"),
                                           &priv->ethernet_switch);
  gtk_box_pack_start (GTK_BOX (box), priv->ethernet_box,
                      FALSE, FALSE, 0);
  g_signal_connect (MX_GTK_LIGHT_SWITCH (priv->ethernet_switch),
                    "switch-flipped",
                    G_CALLBACK (_ethernet_switch_callback),
                    self);

  priv->ethernet_sep = gtk_hseparator_new ();
  gtk_widget_show (priv->ethernet_sep);
  gtk_box_pack_start (GTK_BOX (box), priv->ethernet_sep,
                      FALSE, FALSE, 0);

  priv->threeg_box = get_technology_box (_ ("3G"),
                                         &priv->threeg_switch);
  gtk_box_pack_start (GTK_BOX (box), priv->threeg_box,
                      FALSE, FALSE, 0);
  g_signal_connect (MX_GTK_LIGHT_SWITCH (priv->threeg_switch),
                    "switch-flipped",
                    G_CALLBACK (_threeg_switch_callback),
                    self);

  priv->threeg_sep = gtk_hseparator_new ();
  gtk_widget_show (priv->threeg_sep);
  gtk_box_pack_start (GTK_BOX (box), priv->threeg_sep,
                      FALSE, FALSE, 0);

  priv->wimax_box = get_technology_box (_ ("WiMAX"),
                                        &priv->wimax_switch);
  gtk_box_pack_start (GTK_BOX (box), priv->wimax_box,
                      FALSE, FALSE, 0);
  g_signal_connect (MX_GTK_LIGHT_SWITCH (priv->wimax_switch),
                    "switch-flipped",
                    G_CALLBACK (_wimax_switch_callback),
                    self);

  priv->wimax_sep = gtk_hseparator_new ();
  gtk_widget_show (priv->wimax_sep);
  gtk_box_pack_start (GTK_BOX (box), priv->wimax_sep,
                      FALSE, FALSE, 0);

  priv->bluetooth_box = get_technology_box (_ ("Bluetooth"),
                                            &priv->bluetooth_switch);
  gtk_box_pack_start (GTK_BOX (box), priv->bluetooth_box,
                      FALSE, FALSE, 0);
  g_signal_connect (MX_GTK_LIGHT_SWITCH (priv->bluetooth_switch),
                    "switch-flipped",
                    G_CALLBACK (_bluetooth_switch_callback),
                    self);

  /* wider separator */
  sep = gtk_hseparator_new ();
  gtk_widget_show (sep);
  gtk_box_pack_start (GTK_BOX (column), sep,
                      FALSE, FALSE, 0);

  align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_widget_show (align);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 8, 8);
  gtk_box_pack_start (GTK_BOX (column), align,
                      FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (box);
  gtk_container_add (GTK_CONTAINER (align), box);

  priv->offline_mode_box = get_technology_box (_ ("Offline mode"),
                                               &priv->offline_mode_switch);
  gtk_box_pack_end (GTK_BOX (box), priv->offline_mode_box,
                    FALSE, FALSE, 0);
  g_signal_connect (MX_GTK_LIGHT_SWITCH (priv->offline_mode_switch),
                    "switch-flipped",
                    G_CALLBACK (_offline_mode_switch_callback),
                    self);

  offline_mode_label = gtk_label_new
          (_ ("This will disable all your connections"));
  gtk_label_set_line_wrap (GTK_LABEL (offline_mode_label),
                           TRUE);
  gtk_misc_set_alignment (GTK_MISC (offline_mode_label), 0.0, 0.5);
  gtk_misc_set_padding (GTK_MISC (offline_mode_label), 6, 0);
  gtk_widget_show (offline_mode_label);
  gtk_box_pack_start (GTK_BOX (priv->offline_mode_box),
                      offline_mode_label,
                      FALSE,
                      FALSE,
                      0);
  sep = gtk_hseparator_new ();
  gtk_widget_show (sep);
  gtk_box_pack_start (GTK_BOX (priv->offline_mode_box), sep,
                      FALSE, FALSE, 6);

  gtk_container_add (GTK_CONTAINER (settings_frame), column);
  gtk_box_pack_start (GTK_BOX (self),
                      settings_frame,
                      FALSE,
                      FALSE,
                      0);
  /*
   * End of left column
   */

  /*
   * Right column
   */
  net_list_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (net_list_frame), GTK_SHADOW_OUT);
  gtk_widget_show (net_list_frame);

  column = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (column);

  banner = mux_banner_new (_("Networks"));
  gtk_size_group_add_widget (banner_group, banner);
  gtk_widget_show (banner);
  gtk_box_pack_start (GTK_BOX (column), banner, FALSE, FALSE, 0);

  /* New connection button */
  priv->new_conn_button = gtk_button_new_with_label (_ ("Add new connection"));
  g_signal_connect_after (priv->new_conn_button, "size-request",
                          G_CALLBACK (button_size_request_cb), self);
  gtk_widget_set_sensitive (priv->new_conn_button,
                            FALSE);
  gtk_widget_show (priv->new_conn_button);
  g_signal_connect (GTK_BUTTON (priv->new_conn_button),
                    "clicked",
                    G_CALLBACK (_new_connection_cb),
                    self);
  gtk_box_pack_end (GTK_BOX (banner),
                      priv->new_conn_button,
                      FALSE,
                      FALSE,
                      0);

  /* Network list */
  priv->service_list = carrick_list_new (priv->icon_factory,
                                         priv->notes,
                                         CARRICK_NETWORK_MODEL (model));
  gtk_widget_show (priv->service_list);
  gtk_box_pack_start (GTK_BOX (column), priv->service_list, TRUE, TRUE, 0);

  /* vpn widgets in the bottom */
  priv->vpn_box = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_sensitive (priv->vpn_box, FALSE);
  gtk_widget_set_no_show_all (priv->vpn_box, TRUE);
  gtk_box_pack_start (GTK_BOX (column), priv->vpn_box, FALSE, FALSE, 4);

  sep = gtk_hseparator_new ();
  gtk_widget_show (sep);
  gtk_box_pack_start (GTK_BOX (priv->vpn_box), sep, FALSE, FALSE, 0);

  box = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (priv->vpn_box), box, FALSE, FALSE, 4);

  /* TRANSLATORS: Button
   * There will be a combobox of VPN networks to the left of the 
   * button */
  vpn_connect_button = gtk_button_new_with_label (_("Connect to VPN"));
  gtk_widget_show (vpn_connect_button);
  gtk_box_pack_end (GTK_BOX (box), vpn_connect_button, FALSE, FALSE, 8);
  g_signal_connect (GTK_BUTTON (vpn_connect_button), "clicked",
                    G_CALLBACK (vpn_connect_clicked), self);

  vpn_store = gtk_list_store_new (N_VPN_COLUMNS,
                                  G_TYPE_STRING, G_TYPE_STRING);
  priv->vpn_combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (vpn_store));
  g_object_unref (vpn_store);
  gtk_widget_show (priv->vpn_combo);
  gtk_box_pack_end (GTK_BOX (box), priv->vpn_combo, FALSE, FALSE, 0);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->vpn_combo),
                              renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->vpn_combo),
                                  renderer,
                                  "text", COLUMN_VPN_NAME,
                                  NULL);

  if (g_file_test (AUTH_DIALOG, G_FILE_TEST_IS_EXECUTABLE))
    init_gconf (self);

  gtk_container_add (GTK_CONTAINER (net_list_frame), column);
  gtk_box_pack_start (GTK_BOX (self),
                      net_list_frame,
                      TRUE,
                      TRUE,
                      0);
  /*
   * End right column
   */

  g_signal_connect (GTK_WIDGET (self),
                    "focus",
                    G_CALLBACK (_focus_callback),
                    NULL);
}

void
carrick_pane_update (CarrickPane *pane)
{
  CarrickPanePrivate *priv = pane->priv;
  time_t              now = time (NULL);

  /* Only trigger a scan if we haven't triggered one in the last minute.
   * This number likely needs tweaking.
   */
  if ((now - priv->last_scan) > 60)
    {
      priv->last_scan = now;

      /* Request a scan on all technologies.
       * The UI doesn't really care if the call to scan completes or
       * not so just fire and forget
       */
      net_connman_Manager_request_scan_async (priv->manager,
                                              "",
                                              dbus_proxy_notify_cb,
                                              pane);
    }

  carrick_list_clear_state (CARRICK_LIST (priv->service_list));
}

GtkWidget*
carrick_pane_new (CarrickIconFactory         *icon_factory,
                  CarrickNotificationManager *notifications)
{
  return g_object_new (CARRICK_TYPE_PANE,
                       "icon-factory",
                       icon_factory,
                       "notification-manager",
                       notifications,
                       NULL);
}

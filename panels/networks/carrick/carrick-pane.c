/*
 * Carrick - a connection panel for the Moblin Netbook
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
 *
 */

#include "carrick-pane.h"

#include <config.h>
#include <gconnman/gconnman.h>
#include <nbtk/nbtk-gtk.h>
#include <glib/gi18n.h>
#include "carrick-list.h"
#include "carrick-service-item.h"
#include "carrick-icon-factory.h"
#include "carrick-notification-manager.h"

G_DEFINE_TYPE (CarrickPane, carrick_pane, GTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_PANE, CarrickPanePrivate))

typedef struct _CarrickPanePrivate CarrickPanePrivate;

struct _CarrickPanePrivate {
  CmManager          *manager;
  GtkWidget          *wifi_switch;
  GtkWidget          *wifi_label;
  GtkWidget          *ethernet_switch;
  GtkWidget          *ethernet_label;
  GtkWidget          *threeg_switch;
  GtkWidget          *threeg_label;
  GtkWidget          *wimax_switch;
  GtkWidget          *wimax_label;
  GtkWidget          *bluetooth_switch;
  GtkWidget          *bluetooth_label;
  GtkWidget          *offline_mode_switch;
  GtkWidget          *service_list;
  GtkWidget          *new_conn_button;
  CarrickIconFactory *icon_factory;
  time_t              last_scan;
  gboolean            have_daemon;
  gboolean            have_wifi;
  gboolean            have_ethernet;
  gboolean            have_threeg;
  gboolean            have_wimax;
  gboolean            have_bluetooth;
  gboolean            wifi_enabled;
  gboolean            ethernet_enabled;
  gboolean            threeg_enabled;
  gboolean            wimax_enabled;
  gboolean            bluetooth_enabled;
  CarrickNotificationManager *notes;
};

enum
{
  PROP_0,
  PROP_ICON_FACTORY,
  PROP_MANAGER,
  PROP_NOTIFICATIONS
};

static void _update_manager (CarrickPane *pane,
                             CmManager   *manager);

static gboolean
_focus_callback (GtkWidget *widget, 
		 GtkDirectionType arg1,
		 gpointer user_data)
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
  CarrickPane *pane = CARRICK_PANE (object);
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  switch (property_id) {
  case PROP_ICON_FACTORY:
    g_value_set_object (value, priv->icon_factory);
    break;
  case PROP_MANAGER:
    g_value_set_object (value, priv->manager);
    break;
  case PROP_NOTIFICATIONS:
    g_value_set_object (value, priv->notes);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_pane_set_icon_factory (CarrickPane *pane,
                               CarrickIconFactory *icon_factory)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  priv->icon_factory = icon_factory;
  carrick_list_set_icon_factory (CARRICK_LIST (priv->service_list),
                                 icon_factory);
}

static void
carrick_pane_set_notifications (CarrickPane *pane,
                                CarrickNotificationManager *notes)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

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

  switch (property_id) {
  case PROP_ICON_FACTORY:
    carrick_pane_set_icon_factory (
      pane,
      CARRICK_ICON_FACTORY (g_value_get_object (value)));
    break;

  case PROP_MANAGER:
    _update_manager (pane,
                     CM_MANAGER (g_value_get_object (value)));
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
  CarrickPane *pane = CARRICK_PANE (object);
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  if (priv->manager)
  {
    _update_manager (pane, NULL);
  }

  G_OBJECT_CLASS (carrick_pane_parent_class)->dispose (object);
}

static void
carrick_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (carrick_pane_parent_class)->finalize (object);
}

static void
carrick_pane_class_init (CarrickPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (CarrickPanePrivate));

  object_class->get_property = carrick_pane_get_property;
  object_class->set_property = carrick_pane_set_property;
  object_class->dispose = carrick_pane_dispose;
  object_class->finalize = carrick_pane_finalize;

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

  pspec = g_param_spec_object ("manager",
                               "Manager.",
                               "The gconnman manager to use.",
                               CM_TYPE_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_MANAGER,
                                   pspec);
}

static gboolean
_wifi_switch_callback (NbtkGtkLightSwitch *wifi_switch,
                       gboolean            new_state,
                       CarrickPane        *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  if (new_state)
  {
    carrick_notification_manager_queue_event (priv->notes,
                                              "wifi",
                                              "ready",
                                              "all");
    cm_manager_enable_technology (priv->manager, "wifi");
  }
  else
  {
    carrick_notification_manager_queue_event (priv->notes,
                                              "wifi",
                                              "idle",
                                              "all");
    cm_manager_disable_technology (priv->manager, "wifi");
  }

  return TRUE;
}

static gboolean
_ethernet_switch_callback (NbtkGtkLightSwitch *ethernet_switch,
                           gboolean            new_state,
                           CarrickPane        *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  if (new_state)
  {
    carrick_notification_manager_queue_event (priv->notes,
                                              "ethernet",
                                              "ready",
                                              "all");
    cm_manager_enable_technology (priv->manager, "ethernet");
  }
  else
  {
    carrick_notification_manager_queue_event (priv->notes,
                                              "ethernet",
                                              "idle",
                                              "all");
    cm_manager_disable_technology (priv->manager, "ethernet");
  }

  return TRUE;
}

static gboolean
_threeg_switch_callback (NbtkGtkLightSwitch *threeg_switch,
                         gboolean            new_state,
                         CarrickPane        *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  if (new_state)
  {
    carrick_notification_manager_queue_event (priv->notes,
                                              "cellular",
                                              "ready",
                                              "all");

    cm_manager_enable_technology (priv->manager, "cellular");
  }
  else
  {
    carrick_notification_manager_queue_event (priv->notes,
                                              "cellular",
                                              "idle",
                                              "all");
    cm_manager_disable_technology (priv->manager, "cellular");
  }

  return TRUE;
}

static gboolean
_wimax_switch_callback (NbtkGtkLightSwitch *wimax_switch,
                        gboolean            new_state,
                        CarrickPane        *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  if (new_state)
  {
    carrick_notification_manager_queue_event (priv->notes,
                                              "wimax",
                                              "ready",
                                              "all");
    cm_manager_enable_technology (priv->manager, "wimax");
  }
  else
  {
    carrick_notification_manager_queue_event (priv->notes,
                                              "wimax",
                                              "idle",
                                              "all");
    cm_manager_disable_technology (priv->manager, "wimax");
  }

  return TRUE;
}

static gboolean
_bluetooth_switch_callback (NbtkGtkLightSwitch *bluetooth_switch,
			    gboolean            new_state,
			    CarrickPane        *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  if (new_state)
  {
    carrick_notification_manager_queue_event (priv->notes,
                                              "bluetooth",
                                              "ready",
                                              "all");
    cm_manager_enable_technology (priv->manager, "bluetooth");
  }
  else
  {
    carrick_notification_manager_queue_event (priv->notes,
                                              "bluetooth",
                                              "idle",
                                              "all");
    cm_manager_disable_technology (priv->manager, "bluetooth");
  }

  return TRUE;
}

static void
_secret_check_toggled(GtkToggleButton *toggle,
                      gpointer         user_data)
{
  GtkEntry *entry = GTK_ENTRY(user_data);
  gboolean vis = gtk_toggle_button_get_active(toggle);
  gtk_entry_set_visibility(entry, vis);
}

/*
 * Find the uppermost parent window plug so that
 * we can hide it.
 */
static GtkWidget *
pane_find_plug (GtkWidget *widget)
{
  /* Pippinated */
  while (widget)
  {
    if (GTK_IS_PLUG (widget))
      return widget;
    widget = gtk_widget_get_parent (widget);
  }
  return NULL;
}

static void
_new_connection_cb (GtkButton *button,
                    gpointer   user_data)
{
  CarrickPanePrivate *priv = GET_PRIVATE (user_data);
  GtkWidget *dialog;
  GtkWidget *desc;
  GtkWidget *ssid_entry, *ssid_label;
  GtkWidget *security_combo, *security_label;
  GtkWidget *secret_entry, *secret_label;
  GtkWidget *secret_check;
  GtkWidget *table;
  GtkWidget *tmp;
  const gchar *network, *secret;
  gchar *security;
  GtkWidget *image;
  gboolean joined = FALSE;

  dialog = gtk_dialog_new_with_buttons (_("New connection settings"),
                                        GTK_WINDOW (gtk_widget_get_parent (user_data)),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_REJECT,
                                        GTK_STOCK_CONNECT,
                                        GTK_RESPONSE_ACCEPT, NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog),
                                FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   GTK_RESPONSE_ACCEPT);
  gtk_window_set_icon_name (GTK_WINDOW(dialog),
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

  desc = gtk_label_new (_("Enter the name of the network you want\nto "
                          "connect to and, where necessary, the\n"
                          "password and security type."));
  gtk_table_attach_defaults (GTK_TABLE (table),
                             desc,
                             1, 3,
                             0, 1);
  gtk_misc_set_alignment (GTK_MISC (desc),
                          0.0, 0.5);

  ssid_label = gtk_label_new (_("Network name"));
  gtk_misc_set_alignment (GTK_MISC (ssid_label),
                          0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             ssid_label,
                             1, 2, 1, 2);

  ssid_entry = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table),
                             ssid_entry,
                             2, 3, 1, 2);

  security_label = gtk_label_new (_("Network security"));
  gtk_misc_set_alignment (GTK_MISC (security_label),
                          0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             security_label,
                             1, 2,
                             2, 3);

  security_combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                             _("None"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                             "WEP");
  gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                             "WPA");
  gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                            "WPA2");
  gtk_combo_box_set_active (GTK_COMBO_BOX (security_combo),
                            0);
  gtk_table_attach_defaults (GTK_TABLE(table),
                             security_combo,
                             2, 3,
                             2, 3);

  secret_label = gtk_label_new (_("Password"));
  gtk_misc_set_alignment (GTK_MISC (secret_label),
                          0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table),
                             secret_label,
                             1, 2,
                             3, 4);
  secret_entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (secret_entry),
                            FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             secret_entry,
                             2, 3,
                             3, 4);

  secret_check = gtk_check_button_new_with_label (_("Show password"));
  g_signal_connect (secret_check,
                    "toggled",
                    G_CALLBACK (_secret_check_toggled),
                    (gpointer) secret_entry);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             secret_check,
                             1, 2,
                             4, 5);

  gtk_widget_show_all (dialog);
  tmp = pane_find_plug (GTK_WIDGET (button));
  if (tmp)
  {
    gtk_widget_hide (tmp);
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    network = gtk_entry_get_text (GTK_ENTRY (ssid_entry));
    security = gtk_combo_box_get_active_text (GTK_COMBO_BOX (security_combo));
    secret = gtk_entry_get_text (GTK_ENTRY (secret_entry));

    if (network == NULL)
      return;

    if (gtk_combo_box_get_active (GTK_COMBO_BOX (security_combo)) == 0)
    {
      /* The 0th item is the None entry. Because we have marked that string
       * for translation we should convert it to a form that ConnMan recognises
       * if it has been selected */
      g_free (security);
      security = g_strdup ("none");
    }
    else if(g_strcmp0 (security, "WPA2") == 0)
    {
      g_free (security);
      security = g_strdup ("rsn");
    }
    else
    {
      guint i;
      for (i = 0; security[i] != '\0'; i++)
      {
        security[i] = g_ascii_tolower (security[i]);
      }
    }

    carrick_notification_manager_queue_event (priv->notes,
                                              "wifi",
                                              "ready",
                                              network);
    joined = cm_manager_connect_wifi (priv->manager,
                                      network,
                                      security,
                                      secret);

    /* Need some error handling here */
    if (!joined)
    {
      g_debug ("No WiFi device found to add network...)");
    }
  }
  gtk_widget_destroy (dialog);
}

static void
_service_updated_cb (CmService   *service,
                     CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  CarrickList *list = CARRICK_LIST (priv->service_list);

  /*
   * We only want this to be called for the first
   * 'service-update' signal
   */
  g_signal_handlers_disconnect_by_func (service,
					_service_updated_cb,
					pane);

  /*
   * If we do have a race then do not create multipe service 
   * items for the same service
   */
  if (carrick_list_find_service_item (list, service) == NULL)
  {
    carrick_list_add_item (list, service);
    carrick_list_sort_list (CARRICK_LIST (priv->service_list));

  }
}

static gboolean
_offline_mode_switch_callback (NbtkGtkLightSwitch *flight_switch,
			       gboolean            new_state,
			       CarrickPane        *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  /* FIXME: This is a band aid, needs a better fix */
  carrick_notification_manager_queue_event (priv->notes,
                                            "wifi",
                                            "idle",
                                            "all");
  cm_manager_set_offline_mode (priv->manager, new_state);

  return TRUE;
}

static void
_add_fallback (CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  gchar *fallback = NULL;

  /* Need to add some fall-back content */
  if (!priv->have_daemon)
  {
    /* 
     * Hint to display when we detect that the connection manager 
     * is dead.  Ideally the system auto-restarts connman so the
     * user will not see this, but this is what to show if all
     * available recovery measures fail.
     */
    fallback = g_strdup (_("Sorry, we can't find any networks. "
			   "The Connection Manager doesn't seem to be running. "
			   "You may want to try re-starting your device."
			   ));
  }
  else if (cm_manager_get_offline_mode (priv->manager))
  {
    /* 
     * Hint display if we detect that the system is in
     * offline mode and there are no available networks
     */
    fallback = g_strdup (_("Sorry, we can't find any networks. "
			   "You could try disabling Offline mode. "
			   ));
  }
  else if ((priv->have_wifi && !priv->wifi_enabled) ||
	   (priv->have_ethernet && !priv->ethernet_enabled) ||
	   (priv->have_threeg && !priv->threeg_enabled) ||
	   (priv->have_wimax && !priv->wimax_enabled) ||
	   (priv->have_bluetooth && !priv->bluetooth_enabled))
  {
    if (priv->have_wifi && !priv->wifi_enabled)
    {
      /* 
       * Hint to display if we detect that wifi has been turned off 
       * and there are no available networks
       */
      fallback = g_strdup (_("Sorry, we can't find any networks. "
			     "You could try turning on WiFi."
			     ));
    }
    else if (priv->have_wimax && !priv->wimax_enabled)
    {
      /* 
       * Hint to display if we detect that wifi is on but
       * WiMAX has been turned off and there are no available
       * networks
       */
      fallback = g_strdup (_("Sorry, we can't find any networks. "
			     "You could try turning on WiMAX."
			     ));
    }
    else if (priv->have_threeg && !priv->threeg_enabled)
    {
      /* 
       * Hint to display if we detect that wifi and wimax is on but
       * 3G has been turned off and there are no available
       * networks
       */
      fallback = g_strdup (_("Sorry, we can't find any networks. "
			     "You could try turning on 3G."
			     ));
    }
    else if (priv->have_bluetooth && !priv->bluetooth_enabled)
   {
      /* 
       * Hint to display if we detect that wifi, wimax, and 3G are on but
       * bluetooth has been turned off and there are no available networks
       */
      fallback = g_strdup (_("Sorry, we can't find any networks. "
			     "You could try turning on Bluetooth."
			     ));
    }
    else if (priv->have_ethernet && !priv->ethernet_enabled)
    {
      /* 
       * Hint to display if we detect that all technologies
       * other then ethernet have been turned on, and there
       * are no available networks
       */
      fallback = g_strdup (_("Sorry, we can't find any networks. "
			     "You could try turning on Wired."
			     ));
    }
  }
  else
  {
    /*
     * Generic message to display if all available networking 
     * technologies are turned on, but for whatever reason we
     * can not find any networks
     */
    fallback = g_strdup (_("Sorry, we can't find any networks"));
  }

  carrick_list_set_fallback (CARRICK_LIST (priv->service_list), fallback);
  g_free (fallback);
}

static void
_update_services (CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  CmService *service = NULL;
  const GList *it, *iter;
  const GList *fetched_services = NULL;
  GList *children = NULL;
  gboolean found = FALSE;
  GtkWidget *service_item = NULL;

  fetched_services = cm_manager_get_services (priv->manager);

  children = carrick_list_get_children (CARRICK_LIST (priv->service_list));

  /*
   * Walk the list and the children of the container and:
   * 1. Find stale services, delete widgetry
   * 2. Find new services, add new widgetry
   */
  

  for (it = children; it != NULL; it = it->next)
  {
    service = carrick_service_item_get_service
      (CARRICK_SERVICE_ITEM (it->data));

    if (!service)
      continue;

    for (iter = fetched_services; iter != NULL && !found; iter = iter->next)
    {
      if (!iter->data)
	continue;

      if (cm_service_is_same (service, CM_SERVICE (iter->data)))
      {
        found = TRUE;
      }
    }

    if (!found)
    {
      gtk_widget_destroy (GTK_WIDGET (it->data));
    }
    found = FALSE;
  }

  carrick_list_sort_list (CARRICK_LIST (priv->service_list));

  for (it = fetched_services; it != NULL; it = it->next)
  {
    service = CM_SERVICE (it->data);
    service_item = carrick_list_find_service_item
      (CARRICK_LIST (priv->service_list),
       service);

    if (service_item == NULL)
    {
      g_signal_connect (service,
                        "service-updated",
                        G_CALLBACK (_service_updated_cb),
                        pane);
    }
  }

  if (!fetched_services)
  {
    _add_fallback (pane);
  }
}

static void
_services_changed_cb (CmManager *manager,
                      gpointer   user_data)
{
  _update_services (CARRICK_PANE (user_data));
}

static void
_available_technologies_changed_cb (CmManager *manager,
				    gpointer   user_data)
{
  CarrickPanePrivate *priv = GET_PRIVATE (user_data);
  const GList *l = cm_manager_get_available_technologies (manager);

  priv->have_wifi = FALSE;
  priv->have_ethernet = FALSE;
  priv->have_threeg = FALSE;
  priv->have_wimax = FALSE;
  priv->have_bluetooth = FALSE;

  while (l != NULL)
  {
    const gchar *t = l->data;

    if (g_strcmp0 (t, "ethernet") == 0)
    {
      priv->have_ethernet = TRUE;
    }
    else if (g_strcmp0 (t, "wifi") == 0)
    {
      priv->have_wifi = TRUE;
    }
    else if (g_strcmp0 (t, "threeg") == 0)
    {
      priv->have_threeg = TRUE;
    }
    else if (g_strcmp0 (t, "wimax") == 0)
    {
      priv->have_wimax = TRUE;
    }
    else if (g_strcmp0 (t, "bluetooth") == 0)
    {
      priv->have_bluetooth = TRUE;
    }

    l = l->next;
  }

  gtk_widget_set_sensitive (priv->wifi_switch,
			    priv->have_wifi);
  gtk_widget_set_sensitive (priv->ethernet_switch,
			    priv->have_ethernet);
  gtk_widget_set_sensitive (priv->threeg_switch,
			    priv->have_threeg);
  gtk_widget_set_sensitive (priv->wimax_switch,
			    priv->have_wimax);
  gtk_widget_set_sensitive (priv->bluetooth_switch,
			    priv->have_bluetooth);
}

static void
_enabled_technologies_changed_cb (CmManager *manager,
				  gpointer   user_data)
{
  CarrickPanePrivate *priv = GET_PRIVATE (user_data);
  const GList *l = cm_manager_get_enabled_technologies (manager);

  priv->wifi_enabled = FALSE;
  priv->ethernet_enabled = FALSE;
  priv->threeg_enabled = FALSE;
  priv->wimax_enabled = FALSE;
  priv->bluetooth_enabled = FALSE;

  while (l != NULL)
  {
    const gchar *t = l->data;

    if (g_strcmp0 (t, "ethernet") == 0)
    {
      priv->ethernet_enabled = TRUE;
    } 
    else if (g_strcmp0 (t, "wifi") == 0)
    {
      priv->wifi_enabled = TRUE;
    }
    else if (g_strcmp0 (t, "threeg") == 0)
    {
      priv->threeg_enabled = TRUE;
    }
    else if (g_strcmp0 (t, "wimax") == 0)
    {
      priv->wimax_enabled = TRUE;
    }
    else if (g_strcmp0 (t, "bluetooth") == 0)
    {
      priv->bluetooth_enabled = TRUE;
    }
    
    l = l->next;
  }

  /* disarm signal handlers */
  g_signal_handlers_disconnect_by_func (priv->ethernet_switch,
					_ethernet_switch_callback,
					user_data);
  g_signal_handlers_disconnect_by_func (priv->wifi_switch,
					_wifi_switch_callback,
					user_data);
  g_signal_handlers_disconnect_by_func (priv->threeg_switch,
					_threeg_switch_callback,
					user_data);
  g_signal_handlers_disconnect_by_func (priv->wimax_switch,
					_wimax_switch_callback,
					user_data);
  g_signal_handlers_disconnect_by_func (priv->bluetooth_switch,
					_bluetooth_switch_callback,
					user_data);
  
  nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH
				    (priv->ethernet_switch),
				    priv->ethernet_enabled);
  nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH
				    (priv->wifi_switch),
				    priv->wifi_enabled);
  nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH
				    (priv->threeg_switch),
				    priv->threeg_enabled);
  nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH
				    (priv->wimax_switch),
				    priv->wimax_enabled);
  nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH
				    (priv->bluetooth_switch),
				    priv->bluetooth_enabled);

  /* arm signal handlers */
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->ethernet_switch),
		    "switch-flipped",
		    G_CALLBACK (_ethernet_switch_callback),
		    user_data);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->wifi_switch),
		    "switch-flipped",
		    G_CALLBACK (_wifi_switch_callback),
		    user_data);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->threeg_switch),
		    "switch-flipped",
		    G_CALLBACK (_threeg_switch_callback),
		    user_data);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->wimax_switch),
		    "switch-flipped",
		    G_CALLBACK (_wimax_switch_callback),
		    user_data);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->bluetooth_switch),
		    "switch-flipped",
		    G_CALLBACK (_bluetooth_switch_callback),
		    user_data);

  /* only enable if wifi is enabled */ 
  gtk_widget_set_sensitive (priv->new_conn_button,
			    priv->wifi_enabled);

  if (!cm_manager_get_services (priv->manager))
  {
    _add_fallback (CARRICK_PANE (user_data));
  }
}

static void
_offline_mode_changed_cb (CmManager *manager,
			  gpointer   user_data)
{
  CarrickPanePrivate *priv = GET_PRIVATE (user_data);
  gboolean mode = cm_manager_get_offline_mode (priv->manager);

  /* disarm signal handler */
  g_signal_handlers_disconnect_by_func (priv->offline_mode_switch,
					_offline_mode_switch_callback,
					user_data);

  nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH
				    (priv->offline_mode_switch),
				    mode);

  /* arm signal handler */
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->offline_mode_switch),
                    "switch-flipped",
                    G_CALLBACK (_offline_mode_switch_callback),
                    user_data);
}

static void
_manager_state_changed_cb (CmManager *manager,
                           gpointer   user_data)
{
  CarrickPane *pane = CARRICK_PANE (user_data);
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  const gchar *state = cm_manager_get_state (manager);

  if (g_strcmp0 (state, "unavailable") == 0)
  {
    /* Daemon has gone ... */
    priv->have_daemon = FALSE;
    _update_services (pane);
  }
  else if (g_strcmp0 (state, "offline") == 0)
  {
    priv->have_daemon = TRUE;
    _add_fallback (pane);
  }
}

static void
_update_manager (CarrickPane *pane,
                 CmManager   *manager)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  if (priv->manager)
  {
    g_signal_handlers_disconnect_by_func (priv->manager,
                                          _available_technologies_changed_cb,
                                          pane);
    g_signal_handlers_disconnect_by_func (priv->manager,
                                          _enabled_technologies_changed_cb,
                                          pane);
    g_signal_handlers_disconnect_by_func (priv->manager,
                                          _services_changed_cb,
                                          pane);
    g_signal_handlers_disconnect_by_func (priv->manager,
                                          _manager_state_changed_cb,
                                          pane);

    g_object_unref (priv->manager);
    priv->manager = NULL;
  }

  if (manager)
  {
    priv->manager = g_object_ref (manager);
    g_signal_connect (priv->manager,
                      "services-changed",
                      G_CALLBACK (_services_changed_cb),
                      pane);
    g_signal_connect (priv->manager,
                      "state-changed",
                      G_CALLBACK (_manager_state_changed_cb),
                      pane);
    g_signal_connect (priv->manager,
                      "available-technologies-changed",
                      G_CALLBACK (_available_technologies_changed_cb),
                      pane);
    g_signal_connect (priv->manager,
                      "enabled-technologies-changed",
                      G_CALLBACK (_enabled_technologies_changed_cb),
                      pane);
    g_signal_connect (priv->manager,
                      "offline-mode-changed",
                      G_CALLBACK (_offline_mode_changed_cb),
                      pane);

    _update_services (pane);
  }
}

static void
carrick_pane_init (CarrickPane *self)
{
  CarrickPanePrivate *priv = GET_PRIVATE (self);
  GtkWidget *switch_bin;
  GtkWidget *flight_bin;
  GtkWidget *net_list_bin;
  GtkWidget *hbox, *switch_box;
  GtkWidget *vbox;
  GtkWidget *switch_label;
  GtkWidget *frame_title;
  GtkWidget *offline_mode_label;
  gchar *label = NULL;

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

  switch_bin = nbtk_gtk_frame_new ();
  gtk_widget_show (switch_bin);
  flight_bin = nbtk_gtk_frame_new ();
  gtk_widget_show (flight_bin);
  net_list_bin = nbtk_gtk_frame_new ();
  gtk_widget_show (net_list_bin);

  /* Set table up */
  gtk_table_resize (GTK_TABLE (self),
                    8,
                    6);
  gtk_table_set_homogeneous (GTK_TABLE (self),
                             TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (self), 4);
  gtk_table_set_row_spacings (GTK_TABLE (self), 4);
  gtk_table_set_col_spacings (GTK_TABLE (self), 4);

  /* Network list */
  label = g_strdup_printf ("<span font_desc=\"Liberation Sans Bold 18px\""
                           "foreground=\"#3e3e3e\">%s</span>",
                           _("Networks"));

  frame_title = gtk_label_new ("");
  gtk_widget_show (frame_title);
  gtk_label_set_markup (GTK_LABEL (frame_title),
                        label);
  g_free (label);
  gtk_frame_set_label_widget (GTK_FRAME (net_list_bin),
                              frame_title);

  priv->service_list = carrick_list_new (priv->icon_factory,
                                         priv->notes);
  gtk_container_add (GTK_CONTAINER (net_list_bin),
                     priv->service_list);
  gtk_widget_show (priv->service_list);

  gtk_table_attach_defaults (GTK_TABLE (self),
                             net_list_bin,
                             0, 4,
                             0, 7);

  /* New connection button */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_table_attach (GTK_TABLE (self),
                    vbox,
                    0, 6,
                    7, 8,
                    GTK_FILL,
                    GTK_EXPAND,
                    0, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox,
                      FALSE,
                      TRUE,
                      0);

  priv->new_conn_button = gtk_button_new_with_label (_("Add new connection"));
  gtk_widget_set_sensitive (priv->new_conn_button,
                            FALSE);
  gtk_widget_show (priv->new_conn_button);
  g_signal_connect (GTK_BUTTON (priv->new_conn_button),
                    "clicked",
                    G_CALLBACK (_new_connection_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->new_conn_button,
                      FALSE,
                      TRUE,
                      12);


  /* Switches */
  vbox = gtk_vbox_new (TRUE,
                       6);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (switch_bin),
                     vbox);

  switch_box = gtk_hbox_new (TRUE,
                             6);
  gtk_widget_show (switch_box);

  priv->wifi_switch = nbtk_gtk_light_switch_new ();
  priv->wifi_label = gtk_label_new (_("WiFi"));
  gtk_misc_set_alignment (GTK_MISC (priv->wifi_label),
                          0.2,
                          0.5);
  gtk_widget_show (priv->wifi_switch);
  gtk_widget_show (priv->wifi_label);
  gtk_widget_set_sensitive (priv->wifi_switch, FALSE);

  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->wifi_label,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->wifi_switch,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (vbox),
                      switch_box,
                      FALSE,
                      FALSE,
                      8);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->wifi_switch),
		    "switch-flipped",
		    G_CALLBACK (_wifi_switch_callback),
		    self);

  switch_box = gtk_hbox_new (TRUE,
                             6);
  gtk_widget_show (switch_box);
  priv->ethernet_switch = nbtk_gtk_light_switch_new ();
  priv->ethernet_label = gtk_label_new (_("Wired"));
  gtk_misc_set_alignment (GTK_MISC (priv->ethernet_label),
                          0.2,
                          0.5);
  gtk_widget_show (priv->ethernet_switch);
  gtk_widget_show (priv->ethernet_label);
  gtk_widget_set_sensitive (priv->ethernet_switch, FALSE);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->ethernet_label,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->ethernet_switch,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (vbox),
                      switch_box,
                      FALSE,
                      FALSE,
                      8);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->ethernet_switch),
		    "switch-flipped",
		    G_CALLBACK (_ethernet_switch_callback),
		    self);

  switch_box = gtk_hbox_new (TRUE,
                            6);
  gtk_widget_show (switch_box);
  priv->threeg_switch = nbtk_gtk_light_switch_new ();
  priv->threeg_label = gtk_label_new (_("3G"));
  gtk_misc_set_alignment (GTK_MISC (priv->threeg_label),
                          0.2,
                          0.5);
  gtk_widget_show (priv->threeg_switch);
  gtk_widget_show (priv->threeg_label);
  gtk_widget_set_sensitive (priv->threeg_switch, FALSE);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->threeg_label,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->threeg_switch,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (vbox),
                      switch_box,
                      FALSE,
                      FALSE,
                      8);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->threeg_switch),
		    "switch-flipped",
		    G_CALLBACK (_threeg_switch_callback),
		    self);

  switch_box = gtk_hbox_new (TRUE,
                             6);
  gtk_widget_show (switch_box);
  priv->wimax_switch = nbtk_gtk_light_switch_new ();
  priv->wimax_label = gtk_label_new (_("WiMAX"));
  gtk_misc_set_alignment (GTK_MISC (priv->wimax_label),
                          0.2,
                          0.5);
  gtk_widget_show (priv->wimax_switch);
  gtk_widget_show (priv->wimax_label);
  gtk_widget_set_sensitive (priv->wimax_switch, FALSE);

  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->wimax_label,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->wimax_switch,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (vbox),
                      switch_box,
                      FALSE,
                      FALSE,
                      8);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->wimax_switch),
		    "switch-flipped",
		    G_CALLBACK (_wimax_switch_callback),
		    self);

  switch_box = gtk_hbox_new (TRUE,
                             6);
  gtk_widget_show (switch_box);
  priv->bluetooth_switch = nbtk_gtk_light_switch_new ();
  priv->bluetooth_label = gtk_label_new (_("Bluetooth"));
  gtk_misc_set_alignment (GTK_MISC (priv->bluetooth_label),
                          0.2,
                          0.5);
  gtk_widget_show (priv->bluetooth_switch);
  gtk_widget_show (priv->bluetooth_label);
  gtk_widget_set_sensitive (priv->bluetooth_switch, FALSE);

  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->bluetooth_label,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->bluetooth_switch,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (vbox),
                      switch_box,
                      FALSE,
                      FALSE,
                      8);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->bluetooth_switch),
		    "switch-flipped",
		    G_CALLBACK (_bluetooth_switch_callback),
		    self);

  gtk_table_attach_defaults (GTK_TABLE (self),
                             switch_bin,
                             4, 6,
                             0, 5);

  vbox = gtk_vbox_new (TRUE,
                       0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (flight_bin),
                     vbox);
  priv->offline_mode_switch = nbtk_gtk_light_switch_new ();
  gtk_widget_show (priv->offline_mode_switch);
  switch_box = gtk_hbox_new (TRUE,
                             6);
  gtk_widget_show (switch_box);
  switch_label = gtk_label_new (_("Offline mode"));
  gtk_widget_show (switch_label);
  gtk_misc_set_alignment (GTK_MISC (switch_label),
                          0.2,
                          0.5);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->offline_mode_switch),
                    "switch-flipped",
                    G_CALLBACK (_offline_mode_switch_callback),
                    self);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      switch_label,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->offline_mode_switch,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (vbox),
                      switch_box,
                      TRUE,
                      FALSE,
                      8);
  offline_mode_label = gtk_label_new
    (_("This will disable all your connections"));
  gtk_label_set_line_wrap (GTK_LABEL (offline_mode_label),
                           TRUE);
  gtk_misc_set_alignment (GTK_MISC (offline_mode_label),
                          0.5,
                          0.0);
  gtk_widget_show (offline_mode_label);
  gtk_box_pack_start (GTK_BOX (vbox),
                      offline_mode_label,
                      TRUE,
                      TRUE,
                      0);
  gtk_table_attach_defaults (GTK_TABLE (self),
                             flight_bin,
                             4, 6,
                             5, 7);

  g_signal_connect (GTK_WIDGET (self),
		    "focus",
		    G_CALLBACK (_focus_callback),
		    NULL);

}

void
carrick_pane_update (CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  time_t now = time (NULL);

  /* Only trigger a scan if we haven't triggered one in the last minute.
   * This number likely needs tweaking.
   */
  if (difftime (now, priv->last_scan) > 60)
  {
    priv->last_scan = now;

    cm_manager_request_scan (priv->manager);
  }

  carrick_list_set_all_inactive (CARRICK_LIST (priv->service_list));
}

GtkWidget*
carrick_pane_new (CarrickIconFactory         *icon_factory,
                  CarrickNotificationManager *notifications,
                  CmManager                  *manager)
{
  return g_object_new (CARRICK_TYPE_PANE,
                       "icon-factory",
                       icon_factory,
                       "notification-manager",
                       notifications,
                       "manager",
                       manager,
                       NULL);
}

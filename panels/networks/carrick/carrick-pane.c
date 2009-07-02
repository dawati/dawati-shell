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
  GtkWidget          *flight_mode_switch;
  GtkWidget          *flight_mode_label;
  GtkWidget          *service_list;
  GtkWidget          *new_conn_button;
  CarrickIconFactory *icon_factory;
  time_t              last_scan;
};

enum
{
  PROP_0,
  PROP_ICON_FACTORY,
  PROP_MANAGER
};

static void _update_manager (CarrickPane *pane,
                             CmManager   *manager);

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
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_pane_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CarrickPane *pane = CARRICK_PANE (object);
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  switch (property_id) {
  case PROP_ICON_FACTORY:
    priv->icon_factory = CARRICK_ICON_FACTORY (g_value_get_object (value));
    break;

  case PROP_MANAGER:
    _update_manager (pane,
                     CM_MANAGER (g_value_get_object (value)));
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

  pspec = g_param_spec_object ("manager",
                               "Manager.",
                               "The gconnman manager to use.",
                               CM_TYPE_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_MANAGER,
                                   pspec);
}

static void
_set_devices_state (gchar       *device_type,
                    gboolean     state,
                    CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  const GList *devices = cm_manager_get_devices (priv->manager);

  while (devices)
  {
    CmDevice *device = CM_DEVICE (devices->data);
    CmDeviceType type = cm_device_get_type (device);

    if (g_strcmp0 (device_type, cm_device_type_to_string (type)) == 0)
    {
      cm_device_set_powered (device, state);
      if (state)
      {
        cm_device_scan (device);
      }
    }
    devices = devices->next;
  }
}

static gboolean
_wifi_switch_callback (NbtkGtkLightSwitch *wifi_switch,
                       gboolean            new_state,
                       CarrickPane        *pane)
{
  _set_devices_state (g_strdup ("Wireless"),
                      new_state,
                      pane);

  return TRUE;
}

static gboolean
_ethernet_switch_callback (NbtkGtkLightSwitch *ethernet_switch,
                           gboolean            new_state,
                           CarrickPane        *pane)
{
  _set_devices_state (g_strdup ("Ethernet"),
                      new_state,
                      pane);

  return TRUE;
}

static gboolean
_threeg_switch_callback (NbtkGtkLightSwitch *threeg_switch,
                         gboolean            new_state,
                         CarrickPane        *pane)
{
  _set_devices_state (g_strdup ("Cellular"),
                      new_state,
                      pane);

  return TRUE;
}

static gboolean
_wimax_switch_callback (NbtkGtkLightSwitch *wimax_switch,
                        gboolean            new_state,
                        CarrickPane        *pane)
{
  _set_devices_state (g_strdup ("WiMAX"),
                      new_state,
                      pane);

  return TRUE;
}

static void
_secret_check_toggled(GtkToggleButton *toggle,
                      gpointer         user_data)
{
        GtkEntry *entry = GTK_ENTRY(user_data);
        gboolean vis = gtk_toggle_button_get_active(toggle);
        gtk_entry_set_visibility(entry,
                                 vis);
}

/*
 * Find the uppermost parent window plug so that
 * we can hide it.
 */
GtkWidget *
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

void
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
  const GList *devices;
  CmDevice *device;
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
                             "None");
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

    for (devices = cm_manager_get_devices (priv->manager);
         devices != NULL && !joined;
         devices = devices->next)
    {
      device = devices->data;

      if (CM_IS_DEVICE (device))
      {
        CmDeviceType type = cm_device_get_type (device);
        if (type == DEVICE_WIFI)
        {
          if (g_strcmp0 (security, "WPA2") == 0)
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
          joined = cm_device_join_network (device,
                                           network,
                                           security,
                                           secret);
        }
      }
    }
    /* Need some error handling here */
  }
  gtk_widget_destroy (dialog);
}

void
_service_updated_cb (CmService   *service,
                     CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  const gchar *type = NULL;
  CarrickList *list = CARRICK_LIST (priv->service_list);
  GtkWidget *have_service = carrick_list_find_service_item (list,
                                                           service);

  /* If the widgetry for the service exists remove the handler
   * and ensure the list is sorted */
  if (have_service)
  {
    g_signal_handlers_disconnect_by_func (service,
                                          _service_updated_cb,
                                          pane);
  }

  /* Don't display non-favorite ethernet services or services
   * which don't have a name
   */
  if (cm_service_get_name (service) != NULL)
  {
    type = cm_service_get_type (service);

    if (g_strcmp0 ("ethernet", type) == 0
        && cm_service_get_connected (service) == FALSE)
    {
      return;
    }
    else
    {
      g_signal_handlers_disconnect_by_func (service,
                                            _service_updated_cb,
                                            pane);

      GtkWidget *service_item = carrick_service_item_new (priv->icon_factory,
                                                          service);
      carrick_list_add_item (list,
                             service_item);
      carrick_list_sort_list (CARRICK_LIST (priv->service_list));
    }
  }
}

void
_device_updated_cb (CmDevice *device,
                    gpointer  user_data)
{
  CarrickPanePrivate *priv = GET_PRIVATE (user_data);
  CmDeviceType type = cm_device_get_type (device);
  gboolean state;

  if (type != DEVICE_UNKNOWN)
  {
    state = cm_device_get_powered (device);
    switch (type)
    {
      case DEVICE_ETHERNET:
        nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->ethernet_switch),
                                      state);
        gtk_widget_set_sensitive (priv->ethernet_switch,
                                  TRUE);
	gtk_widget_set_no_show_all (priv->ethernet_switch,
				    FALSE);
        gtk_widget_show (priv->ethernet_switch);
        gtk_widget_set_no_show_all (priv->ethernet_label,
				    FALSE);
        gtk_widget_show (priv->ethernet_label);
        g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->ethernet_switch),
                          "switch-flipped",
                          G_CALLBACK (_ethernet_switch_callback),
                          user_data);
        break;
      case DEVICE_WIFI:
        nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->wifi_switch),
                                   state);
        gtk_widget_set_sensitive (priv->wifi_switch,
                                  TRUE);
	gtk_widget_set_no_show_all (priv->wifi_switch,
				    FALSE);
        gtk_widget_show (priv->wifi_switch);
	gtk_widget_set_no_show_all (priv->wifi_label,
				    FALSE);
        gtk_widget_show (priv->wifi_label);
        /* Only enable "Add new connection" button when wifi powered on */
        gtk_widget_set_sensitive (priv->new_conn_button,
                                  state);
        g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->wifi_switch),
                          "switch-flipped",
                          G_CALLBACK (_wifi_switch_callback),
                          user_data);
        break;
      case DEVICE_CELLULAR:
        nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->threeg_switch),
                                   state);
        gtk_widget_set_sensitive (priv->threeg_switch,
                                  TRUE);
	gtk_widget_set_no_show_all (priv->threeg_switch,
				    FALSE);
        gtk_widget_show (priv->threeg_switch);
	gtk_widget_set_no_show_all (priv->threeg_label,
				    FALSE);
        gtk_widget_show (priv->threeg_label);
        g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->threeg_switch),
                          "switch-flipped",
                          G_CALLBACK (_threeg_switch_callback),
                          user_data);
        break;
      case DEVICE_WIMAX:
        nbtk_gtk_light_switch_set_active (NBTK_GTK_LIGHT_SWITCH (priv->wimax_switch),
                                   state);
        gtk_widget_set_sensitive (priv->wimax_switch,
                                  TRUE);
	gtk_widget_set_no_show_all (priv->wimax_switch,
				    FALSE);
        gtk_widget_show (priv->wimax_switch);
	gtk_widget_set_no_show_all (priv->wimax_label,
				    FALSE);
        gtk_widget_show (priv->wimax_label);
        g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->wimax_switch),
                          "switch-flipped",
                          G_CALLBACK (_wimax_switch_callback),
                          user_data);
        break;
      default:
        g_debug ("Unknown device type\n");
        break;
    }
  }
}

static void
_set_states (CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  const GList *devices = NULL;
  const GList *it = NULL;
  CmDevice *device = NULL;

  if (cm_manager_get_offline_mode (priv->manager))
  {
    gtk_widget_set_sensitive (priv->ethernet_switch,
                              FALSE);
    gtk_widget_set_sensitive (priv->wifi_switch,
                              FALSE);
    gtk_widget_set_sensitive (priv->threeg_switch,
                              FALSE);
    gtk_widget_set_sensitive (priv->wimax_switch,
                              FALSE);
  }
  else
  {
    devices = cm_manager_get_devices (priv->manager);
    gtk_widget_hide (priv->wifi_switch);
    gtk_widget_set_no_show_all (priv->wifi_switch,
                                TRUE);
    gtk_widget_hide (priv->wifi_label);
    gtk_widget_set_no_show_all (priv->wifi_label,
                                TRUE);
    gtk_widget_hide (priv->ethernet_switch);
    gtk_widget_set_no_show_all (priv->ethernet_switch,
                                TRUE);
    gtk_widget_hide (priv->ethernet_label);
    gtk_widget_set_no_show_all (priv->ethernet_label,
                                TRUE);
    gtk_widget_hide (priv->threeg_switch);
    gtk_widget_set_no_show_all (priv->threeg_switch,
                                TRUE);
    gtk_widget_hide (priv->threeg_label);
    gtk_widget_set_no_show_all (priv->threeg_label,
                                TRUE);
    gtk_widget_hide (priv->wimax_switch);
    gtk_widget_set_no_show_all (priv->wimax_switch,
                                TRUE);
    gtk_widget_hide (priv->wimax_label);
    gtk_widget_set_no_show_all (priv->wimax_label,
                                TRUE);

    for (it = devices; it != NULL; it = it->next)
    {
      device = CM_DEVICE (it->data);
      g_signal_connect (G_OBJECT (device),
                        "device-updated",
                        G_CALLBACK (_device_updated_cb),
                        pane);
    }
  }
}

static gboolean
_flight_mode_switch_callback (NbtkGtkLightSwitch *flight_switch,
                              gboolean            new_state,
                              CarrickPane        *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  cm_manager_set_offline_mode (priv->manager, new_state);
  _set_states (pane);

  return TRUE;
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
  children = gtk_container_get_children (GTK_CONTAINER (priv->service_list));

  /*
   * Walk the list and the children of the container and:
   * 1. Find stale services, delete widgetry
   * 2. Find new services, add new widgetry
   */
  for (it = children; it != NULL; it = it->next)
  {
    service = carrick_service_item_get_service (
      CARRICK_SERVICE_ITEM (it->data));
    for (iter = fetched_services; iter != NULL; iter = iter->next)
    {
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
}

static void
_services_changed_cb (CmManager *manager,
                      gpointer   user_data)
{
  _update_services (CARRICK_PANE (user_data));
}

static void
_devices_changed_cb (CmManager *manager,
                     gpointer   user_data)
{
  _set_states (CARRICK_PANE (user_data));
}

static void
_update_manager (CarrickPane *pane,
                 CmManager   *manager)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  if (priv->manager)
  {
    g_signal_handlers_disconnect_by_func (priv->manager,
                                          _devices_changed_cb,
                                          pane);
    g_signal_handlers_disconnect_by_func (priv->manager,
                                          _services_changed_cb,
                                          pane);
    g_object_unref (priv->manager);
    priv->manager = NULL;
  }

  if (manager)
  {
    priv->manager = g_object_ref (manager);
    _update_services (pane);
    _set_states (pane);
    g_signal_connect (priv->manager,
                      "devices-changed",
                      G_CALLBACK (_devices_changed_cb),
                      pane);
    g_signal_connect (priv->manager,
                      "services-changed",
                      G_CALLBACK (_services_changed_cb),
                      pane);
  }
}

static void
carrick_pane_init (CarrickPane *self)
{
  CarrickPanePrivate *priv = GET_PRIVATE (self);
  GtkWidget *switch_bin = nbtk_gtk_frame_new ();
  GtkWidget *flight_bin = nbtk_gtk_frame_new ();
  GtkWidget *net_list_bin = nbtk_gtk_frame_new ();
  GtkWidget *scrolled_view;
  GtkWidget *hbox, *switch_box;
  GtkWidget *vbox;
  GtkWidget *switch_label;
  GtkWidget *frame_title;
  gchar *label = NULL;

  priv->icon_factory = NULL;
  priv->manager = NULL;
  priv->last_scan = time (NULL);

  /* Set table up */
  gtk_table_resize (GTK_TABLE (self),
                    8,
                    6);
  gtk_table_set_homogeneous (GTK_TABLE (self),
                             TRUE);

  /* Network list */
  label = g_strdup_printf ("<span font_desc=\"Liberation Sans Bold 18px\""
                           "foreground=\"#3e3e3e\">%s</span>",
                           _("Networks"));

  frame_title = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (frame_title),
                        label);
  g_free (label);
  gtk_frame_set_label_widget (GTK_FRAME (net_list_bin),
                              frame_title);
  priv->service_list = carrick_list_new ();
  scrolled_view = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_view),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_view),
                                         priv->service_list);
  gtk_container_add (GTK_CONTAINER (net_list_bin),
                     scrolled_view);
  gtk_table_attach_defaults (GTK_TABLE (self),
                             net_list_bin,
                             0, 4,
                             0, 7);

  /* New connection button */
  hbox = gtk_hbox_new (FALSE, 0);
  priv->new_conn_button = gtk_button_new_with_label (_("Add new connection"));
  gtk_widget_set_sensitive (priv->new_conn_button,
                            FALSE);
  g_signal_connect (GTK_BUTTON (priv->new_conn_button),
                    "clicked",
                    G_CALLBACK (_new_connection_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->new_conn_button,
                      TRUE,
                      FALSE,
                      0);
  gtk_table_attach (GTK_TABLE (self),
                    hbox,
                    0, 1,
                    7, 8,
                    GTK_EXPAND,
                    GTK_EXPAND,
                    0, 0);

  /* Switches */
  vbox = gtk_vbox_new (TRUE,
                       6);
  gtk_container_add (GTK_CONTAINER (switch_bin),
                     vbox);

  switch_box = gtk_hbox_new (TRUE,
                             6);
  priv->wifi_switch = nbtk_gtk_light_switch_new ();
  gtk_widget_set_sensitive (priv->wifi_switch,
                            FALSE);

  priv->wifi_label = gtk_label_new (_("WiFi"));
  gtk_misc_set_alignment (GTK_MISC (priv->wifi_label),
                          0.2,
                          0.5);
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
                      TRUE,
                      TRUE,
                      8);

  priv->ethernet_switch = nbtk_gtk_light_switch_new ();
  switch_box = gtk_hbox_new (TRUE,
                             6);
  priv->ethernet_label = gtk_label_new (_("Wired"));
  gtk_misc_set_alignment (GTK_MISC (priv->ethernet_label),
                          0.2,
                          0.5);
  gtk_widget_set_sensitive (priv->ethernet_switch,
                            FALSE);

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
                      TRUE,
                      TRUE,
                      8);

  priv->threeg_switch = nbtk_gtk_light_switch_new ();
  switch_box = gtk_hbox_new (TRUE,
                            6);
  priv->threeg_label = gtk_label_new (_("3G"));
  gtk_misc_set_alignment (GTK_MISC (priv->threeg_label),
                          0.2,
                          0.5);
  gtk_widget_set_sensitive (priv->threeg_switch,
                            FALSE);
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
                      TRUE,
                      TRUE,
                      8);

  priv->wimax_switch = nbtk_gtk_light_switch_new ();
  switch_box = gtk_hbox_new (TRUE,
                             6);
  priv->wimax_label = gtk_label_new (_("WiMAX"));
  gtk_misc_set_alignment (GTK_MISC (priv->wimax_label),
                          0.2,
                          0.5);
  gtk_widget_set_sensitive (priv->wimax_switch,
                            FALSE);
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
                      TRUE,
                      TRUE,
                      8);

  gtk_table_attach_defaults (GTK_TABLE (self),
                             switch_bin,
                             4, 6,
                             0, 5);

  vbox = gtk_vbox_new (TRUE,
                       0);
  gtk_container_add (GTK_CONTAINER (flight_bin),
                     vbox);
  priv->flight_mode_switch = nbtk_gtk_light_switch_new ();
  switch_box = gtk_hbox_new (TRUE,
                             6);
  switch_label = gtk_label_new (_("Offline mode"));
  gtk_misc_set_alignment (GTK_MISC (switch_label),
                          0.2,
                          0.5);
  g_signal_connect (NBTK_GTK_LIGHT_SWITCH (priv->flight_mode_switch),
                    "switch-flipped",
                    G_CALLBACK (_flight_mode_switch_callback),
                    self);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      switch_label,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (switch_box),
                      priv->flight_mode_switch,
                      TRUE,
                      TRUE,
                      8);
  gtk_box_pack_start (GTK_BOX (vbox),
                      switch_box,
                      TRUE,
                      FALSE,
                      8);
  priv->flight_mode_label = gtk_label_new
    (_("This will disable all your connections"));
  gtk_label_set_line_wrap (GTK_LABEL (priv->flight_mode_label),
                           TRUE);
  gtk_misc_set_alignment (GTK_MISC (priv->flight_mode_label),
                          0.5,
                          0.0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->flight_mode_label,
                      TRUE,
                      TRUE,
                      0);
  gtk_table_attach_defaults (GTK_TABLE (self),
                             flight_bin,
                             4, 6,
                             5, 7);

}

static void
_set_item_inactive (GtkWidget *widget, gpointer userdata)
{
  if (CARRICK_IS_SERVICE_ITEM (widget))
  {
    carrick_service_item_set_active (CARRICK_SERVICE_ITEM (widget),
                                     FALSE);
  }
}

void
carrick_pane_update (CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  const GList *devices = cm_manager_get_devices (priv->manager);
  CmDevice *dev;
  CmDeviceType type;
  time_t now = time (NULL);

  /* Only trigger a scan if we haven't triggered one in the last minute.
   * This number likely needs tweaking.
   */
  if (difftime (now, priv->last_scan) < 60)
  {
    priv->last_scan = time (NULL);

    while (devices)
    {
      dev = devices->data;
      if (dev && CM_IS_DEVICE (dev))
      {
        type = cm_device_get_type (dev);

        if (type != DEVICE_ETHERNET && type != DEVICE_UNKNOWN)
          cm_device_scan (dev);
      }
      devices = devices->next;
    }
  }

  gtk_container_foreach (GTK_CONTAINER (priv->service_list),
                         (GtkCallback)_set_item_inactive,
                         NULL);
}

GtkWidget*
carrick_pane_new (CarrickIconFactory *icon_factory,
                  CmManager          *manager)
{
  return g_object_new (CARRICK_TYPE_PANE,
                       "icon-factory",
                       icon_factory,
                       "manager",
                       manager,
                       NULL);
}

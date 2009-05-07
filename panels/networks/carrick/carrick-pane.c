/* carrick-pane.c */

#include "carrick-pane.h"

#include <config.h>
#include <gconnman/gconnman.h>
#include <mux/mux-switch-box.h>
#include <mux/mux-bin.h>
#include <glib/gi18n.h>
#include <carrick/carrick-list.h>
#include <carrick/carrick-service-item.h>

G_DEFINE_TYPE (CarrickPane, carrick_pane, GTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_PANE, CarrickPanePrivate))

typedef struct _CarrickPanePrivate CarrickPanePrivate;

struct _CarrickPanePrivate {
  CmManager *manager;
  GList     *services;
  GtkWidget *wifi_switch;
  GtkWidget *ethernet_switch;
  GtkWidget *threeg_switch;
  GtkWidget *wimax_switch;
  GtkWidget *flight_mode_switch;
  GtkWidget *flight_mode_label;
  GtkWidget *service_list;
  GtkWidget *new_conn_button;
};

enum
{
  PROP_0,
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

  switch (property_id) {
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

  pspec = g_param_spec_object ("manager",
                               "Manager.",
                               "The gconnman manager to use.",
                               CM_TYPE_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_MANAGER, pspec);
}

static void
_set_devices_state (gchar       *device_type,
                    gboolean     state,
                    CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  GList *devices = cm_manager_get_devices (priv->manager);

  while (devices) {
    CmDevice *device = devices->data;
    CmDeviceType type = cm_device_get_type (device);

    if (g_strcmp0 (device_type, cm_device_type_to_string (type)) == 0)
    {
      cm_device_set_powered (device, state);
      return;
    }

    devices = g_list_next (devices);
  }

  g_list_free (devices);
}

static gboolean
_wifi_switch_callback (MuxSwitchBox *wifi_switch,
                       gboolean new_state,
                       CarrickPane *pane)
{
  _set_devices_state (g_strdup ("Wireless"),
                      new_state,
                      pane);

  return TRUE;
}

static gboolean
_ethernet_switch_callback (MuxSwitchBox *ethernet_switch,
                           gboolean new_state,
                           CarrickPane *pane)
{
  _set_devices_state (g_strdup ("Ethernet"),
                      new_state,
                      pane);

  return TRUE;
}

static gboolean
_threeg_switch_callback (MuxSwitchBox *threeg_switch,
                         gboolean new_state,
                         CarrickPane *pane)
{
  _set_devices_state (g_strdup ("Cellular"),
                      new_state,
                      pane);

  return TRUE;
}

static gboolean
_wimax_switch_callback (MuxSwitchBox *wimax_switch,
                        gboolean new_state,
                        CarrickPane *pane)
{
  _set_devices_state (g_strdup ("WiMax"),
                      new_state,
                      pane);
}

static gboolean
_flight_mode_switch_callback (MuxSwitchBox *flight_switch,
                              gboolean new_state,
                              CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  cm_manager_set_offline_mode (priv->manager, new_state);

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
  GtkWidget *hbox;
  const gchar *network, *secret;
  gchar *security;
  GtkWidget *image;
  GList *devices;
  CmDevice *device;
  gboolean joined = FALSE;

  dialog = gtk_dialog_new_with_buttons (_("New connection settings"),
                                        GTK_WINDOW (user_data),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_REJECT,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_ACCEPT, NULL);

  gtk_window_set_icon_name(GTK_WINDOW(dialog),
                           GTK_STOCK_DIALOG_AUTHENTICATION);
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), 6);

  table = gtk_table_new(5, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(table), 6);
  gtk_container_set_border_width(GTK_CONTAINER(table), 6);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);
  hbox = gtk_hbox_new (FALSE, 2);
  image = gtk_image_new_from_icon_name(GTK_STOCK_DIALOG_AUTHENTICATION,
                                       GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start(GTK_BOX(hbox), image, TRUE, TRUE, 0);
  desc = gtk_label_new(_("Enter the name of the network you want\nto "
                         "connect to and, where necessary, the\n"
                         "password and security type."));
  gtk_box_pack_start(GTK_BOX(hbox), desc, TRUE, TRUE, 0);

  gtk_table_attach_defaults(GTK_TABLE(table), hbox, 0, 2, 0, 1);

  ssid_label = gtk_label_new(_("Network name"));
  gtk_misc_set_alignment(GTK_MISC(ssid_label), 0.0, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table), ssid_label, 0, 1, 1, 2);
  ssid_entry = gtk_entry_new();
  gtk_table_attach_defaults(GTK_TABLE(table), ssid_entry, 1, 2, 1, 2);

  security_label = gtk_label_new(_("Network security"));
  gtk_misc_set_alignment(GTK_MISC(security_label), 0.0, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table), security_label, 0, 1, 2, 3);

  security_combo = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(security_combo), "None");
  gtk_combo_box_append_text(GTK_COMBO_BOX(security_combo), "WEP");
  gtk_combo_box_append_text(GTK_COMBO_BOX(security_combo), "WPA");
  gtk_combo_box_append_text(GTK_COMBO_BOX(security_combo), "WPA2");
  gtk_combo_box_set_active(GTK_COMBO_BOX(security_combo), 0);
  gtk_table_attach_defaults(GTK_TABLE(table), security_combo, 1, 2, 2, 3);

  secret_label = gtk_label_new(_("Password"));
  gtk_misc_set_alignment(GTK_MISC(secret_label), 0.0, 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table), secret_label, 0, 1, 3, 4);
  secret_entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(secret_entry), FALSE);
  gtk_table_attach_defaults(GTK_TABLE(table), secret_entry, 1, 2, 3, 4);

  secret_check = gtk_check_button_new_with_label(_("Show passphrase"));
  g_signal_connect(secret_check, "toggled",
                   G_CALLBACK(_secret_check_toggled), secret_entry);
  gtk_table_attach_defaults(GTK_TABLE(table), secret_check, 0, 2, 4, 5);

  gtk_widget_show_all(dialog);

  if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    network = gtk_entry_get_text(GTK_ENTRY(ssid_entry));
    security = gtk_combo_box_get_active_text(GTK_COMBO_BOX (security_combo));
    secret = gtk_entry_get_text(GTK_ENTRY(secret_entry));

    if (network == NULL)
      return;

    devices = g_list_copy (cm_manager_get_devices (priv->manager));
    device = CM_DEVICE (g_list_first (devices)->data);
    while (device)
    {
      if (cm_device_get_type (device) == DEVICE_WIFI)
      {
        joined = cm_device_join_network (device,
                                         network,
                                         security,
                                         secret);
      }
      else
      {
        device = CM_DEVICE (g_list_next (devices)->data);
        // FIXME: Handle failure!
      }
    }

    g_list_free (devices);
  }
  gtk_widget_destroy(dialog);
}

void
_service_updated_cb (CmService *service,
                     gpointer   user_data)
{
  CarrickPanePrivate *priv = GET_PRIVATE (user_data);

  if (cm_service_get_name (service))
  {
    GtkWidget *service_item = carrick_service_item_new (service);
    priv->services = g_list_append (priv->services,
                                    (gpointer) service_item);
    carrick_list_add_item (CARRICK_LIST (priv->service_list),
                           service_item);

    g_signal_handlers_disconnect_by_func (service,
                                          _service_updated_cb,
                                          user_data);
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
        mux_switch_box_set_active (MUX_SWITCH_BOX (priv->ethernet_switch),
                                   state);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->ethernet_switch),
                                  TRUE);
        break;
      case DEVICE_WIFI:
        mux_switch_box_set_active (MUX_SWITCH_BOX (priv->wifi_switch),
                                   state);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->wifi_switch),
                                  TRUE);
        break;
      case DEVICE_CELLULAR:
        mux_switch_box_set_active (MUX_SWITCH_BOX (priv->threeg_switch),
                                   state);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->threeg_switch),
                                  TRUE);
        break;
      case DEVICE_WIMAX:
        mux_switch_box_set_active (MUX_SWITCH_BOX (priv->wimax_switch),
                                   state);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->wimax_switch),
                                  TRUE);
        break;
      default:
        g_debug ("Unknown device type\n");
        break;
    }
  }
}

static void
_update_services (CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  GList *raw_services;
  CmService *service;
  guint cnt, len;

  /* Bin existing services, re-creating cheaper than comparing? */
  while (priv->services)
  {
    g_object_unref (priv->services->data);
    priv->services = g_list_delete_link (priv->services,
                                         priv->services);
  }
  raw_services = cm_manager_get_services (priv->manager);
  if (!raw_services) /* FIXME: handle this better */
  {
    return;
  }
  len = g_list_length (raw_services);
  for (cnt = 0; cnt < len; cnt++)
  {
    service = CM_SERVICE (g_list_nth (raw_services, cnt)->data);

    g_signal_connect (G_OBJECT (service),
                      "service-updated",
                      G_CALLBACK (_service_updated_cb),
                      pane);
  }

  g_list_free (raw_services);
}

static void
_set_states (CarrickPane *pane)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);
  GList *devices = cm_manager_get_devices (priv->manager);
  CmDevice *device = NULL;
  guint len;
  guint cnt;

  len = g_list_length (devices);
  for (cnt = 0; cnt < len; cnt++)
  {
    device = CM_DEVICE (g_list_nth (devices, cnt)->data);
    g_signal_connect (G_OBJECT (device),
                      "device-updated",
                      G_CALLBACK (_device_updated_cb),
                      pane);
  }

  g_list_free (devices);
}

static void
_manager_updated_cb (GObject    *object,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
  _set_states (CARRICK_PANE (user_data));
  _update_services (CARRICK_PANE (user_data));
}

static void
_update_manager (CarrickPane *pane,
                 CmManager   *manager)
{
  CarrickPanePrivate *priv = GET_PRIVATE (pane);

  if (manager) {
    g_debug ("Updating manager");
    if (priv->manager)
    {
      g_signal_handlers_disconnect_by_func (priv->manager,
                                            _manager_updated_cb,
                                            pane);
      g_object_unref (priv->manager);
      priv->manager = NULL;
    }

    priv->manager = g_object_ref (manager);
    _update_services (pane);
    _set_states (pane);
    g_signal_connect (priv->manager,
                      "manager-updated",
                      G_CALLBACK (_manager_updated_cb),
                      pane);
  }
}

static void
carrick_pane_init (CarrickPane *self)
{
  CarrickPanePrivate *priv = GET_PRIVATE (self);
  GtkWidget *switch_bin = mux_bin_new ();
  GtkWidget *net_list_bin = mux_bin_new ();
  GtkWidget *scrolled_view;

  /* Set table up */
  gtk_table_resize (GTK_TABLE (self),
                    8,
                    6);
  gtk_table_set_homogeneous (GTK_TABLE (self),
                             TRUE);

  /* Network list */
  mux_bin_set_title (MUX_BIN (net_list_bin),
                    _("Networks"));
  priv->service_list = carrick_list_new ();
  scrolled_view = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_view),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_view),
                                         priv->service_list);
  gtk_container_add (GTK_CONTAINER (net_list_bin),
                     scrolled_view);
  if (priv->manager)
  {
    //_update_services (self);
    //_set_states (self);
    g_signal_connect (priv->manager,
                      "manager-updated",
                      G_CALLBACK (_manager_updated_cb),
                      self);
  }
  gtk_table_attach_defaults (GTK_TABLE (self),
                             net_list_bin,
                             0, 4,
                             0, 7);

  /* New connection button */
  priv->new_conn_button = gtk_button_new_with_label (_("Add new connection"));
  g_signal_connect (GTK_BUTTON (priv->new_conn_button),
                    "clicked",
                    G_CALLBACK (_new_connection_cb),
                    self);
  gtk_table_attach_defaults (GTK_TABLE (self),
                             priv->new_conn_button,
                             0, 1,
                             7, 8);

  /* Switches */
  GtkWidget *vbox = gtk_vbox_new (TRUE,
                                  6);
  gtk_container_add (GTK_CONTAINER (switch_bin),
                     vbox);

  priv->wifi_switch = mux_switch_box_new (_("Wifi"));
  gtk_widget_set_sensitive (GTK_WIDGET (priv->wifi_switch),
                            FALSE);
  g_signal_connect (priv->wifi_switch,
                    "switch-toggled",
                    G_CALLBACK (_wifi_switch_callback),
                    self);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->wifi_switch,
                      TRUE,
                      TRUE,
                      8);

  priv->ethernet_switch = mux_switch_box_new (_("Ethernet"));
  gtk_widget_set_sensitive (GTK_WIDGET (priv->ethernet_switch),
                            FALSE);
  g_signal_connect (priv->ethernet_switch,
                    "switch-toggled",
                    G_CALLBACK (_ethernet_switch_callback),
                    self);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->ethernet_switch,
                      TRUE,
                      TRUE,
                      8);

  priv->threeg_switch = mux_switch_box_new (_("3G"));
  gtk_widget_set_sensitive (GTK_WIDGET (priv->threeg_switch),
                            FALSE);
  g_signal_connect (priv->threeg_switch,
                    "switch-toggled",
                    G_CALLBACK (_threeg_switch_callback),
                    self);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->threeg_switch,
                      TRUE,
                      TRUE,
                      8);
  priv->wimax_switch = mux_switch_box_new (_("WiMax"));
  gtk_widget_set_sensitive (GTK_WIDGET (priv->wimax_switch),
                            FALSE);
  g_signal_connect (priv->wimax_switch,
                    "switch-toggled",
                    G_CALLBACK (_wimax_switch_callback),
                    self);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->wimax_switch,
                      TRUE,
                      TRUE,
                      8);

  priv->flight_mode_switch = mux_switch_box_new (_("Offline Mode"));
  g_signal_connect (priv->flight_mode_switch,
                    "switch-toggled",
                    G_CALLBACK (_flight_mode_switch_callback),
                    self);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->flight_mode_switch,
                      TRUE,
                      FALSE,
                      8);
  priv->flight_mode_label = gtk_label_new (_("This will disable all wireless"
                                             " connections."));
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->flight_mode_label,
                      TRUE,
                      TRUE,
                      0);
  gtk_table_attach_defaults (GTK_TABLE (self),
                             switch_bin,
                             4, 6,
                             0, 5);
}

GtkWidget*
carrick_pane_new (CmManager *manager)
{
  return g_object_new (CARRICK_TYPE_PANE,
                       "manager",
                       manager,
                       NULL);
}

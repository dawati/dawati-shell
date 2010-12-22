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
 * 
 * Written by - Joshua Lock <josh@linux.intel.com>
 *              Jussi Kukkonen <jku@linux.intel.com>
 *
 * Copyright (C) 2009, 2010 Intel Corporation
 *
 */

/* TODO
 * 
 * The _populate_variables() / _set_state() cycle is
 * inefficient and complex. _set_state() updates everything, 
 * even if nothing changed since the last update.
 */

#include "carrick-service-item.h"

#include <config.h>
#include <arpa/inet.h>
#include <glib/gi18n.h>
#include <mx-gtk/mx-gtk.h>
#include "nbtk-gtk-expander.h"

#include "connman-service-bindings.h"

#include "carrick-icon-factory.h"
#include "carrick-notification-manager.h"
#include "carrick-util.h"
#include "carrick-shell.h"

#define CARRICK_DRAG_TARGET "CARRICK_DRAG_TARGET"

static const GtkTargetEntry carrick_targets[] = {
  { CARRICK_DRAG_TARGET, GTK_TARGET_SAME_APP, 0 },
};

G_DEFINE_TYPE (CarrickServiceItem, carrick_service_item, GTK_TYPE_EVENT_BOX)

#define SERVICE_ITEM_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_SERVICE_ITEM, CarrickServiceItemPrivate))

/* Translators:
   The following are potential errors that a user might see while
   attempting to configure a 3G data service.
*/
#define INVALID_COUNTRY_CODE _("Critical Error: Invalid country code")
#define MISSING_APN_NAME _("Missing required APN (service plan name)")
#define INVALID_NETWORK_SERVICE _("Internal Error: Invalid network service")
#define INVALID_CONNMAN _("Critical Error: Unable to access Connection Manager")
#define MISSING_3G_HW _("ERROR: No 3G Hardware detected")

enum {
  PROP_0,
  PROP_FAVORITE,
  PROP_ICON_FACTORY,
  PROP_NOTIFICATIONS,
  PROP_MODEL,
  PROP_ROW,
  PROP_ACTIVE,
};

struct _CarrickServiceItemPrivate
{
  GtkWidget *icon;
  GtkWidget *name_label;
  GtkWidget *connect_box;
  GtkWidget *security_label;
  GtkWidget *portal_button;
  GtkWidget *connect_button;
  GtkWidget *advanced_expander;
  GtkWidget *passphrase_box;
  GtkWidget *passphrase_entry;
  GtkWidget *show_password_check;
  GtkWidget *delete_button;
  GtkWidget *expando;

  GtkWidget *advanced_box;
  GtkWidget *method_combo;
  GtkWidget *address_entry;
  GtkWidget *gateway_entry;
  GtkWidget *netmask_entry;
  GtkWidget *dns_text_view;
  GtkWidget *apply_button;
  gboolean form_modified;

  CarrickIconFactory *icon_factory;
  gboolean failed;
  gboolean passphrase_hint_visible;

  CarrickNotificationManager *note;

  GdkCursor *hand_cursor;

  CarrickNetworkModel *model;
  GtkTreeRowReference *row;

  DBusGProxy *proxy;
  guint index;
  gchar *name;
  gchar *type;
  gchar *state;
  guint  strength;
  gchar   *security;
  gboolean need_pass;
  gchar *passphrase;
  gboolean setup_required;
  gchar *method;
  gchar *address;
  gchar *netmask;
  gchar *gateway;
  gchar **nameservers;
  gboolean favorite;
  gboolean immutable;
  gboolean login_required;

  GtkWidget *info_bar;
  GtkWidget *info_label;

  /* active means the last ServiceItem that has been interacted with
   * since panel was last shown */
  gboolean active;
  /* have we shown and hidden the connect-related error yet? */
  gboolean error_hidden;
};

enum {
  METHOD_DHCP = 0,
  METHOD_MANUAL = 1,
  METHOD_FIXED = 2,
};

typedef enum {
  CARRICK_INET_V4,
  CARRICK_INET_V6,
  CARRICK_INET_ANY
} CarrickInet;

static void
carrick_service_item_get_property (GObject *object, guint property_id,
                                   GValue *value, GParamSpec *pspec)
{
  CarrickServiceItemPrivate *priv;

  g_return_if_fail (object != NULL);
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (object));

  priv = SERVICE_ITEM_PRIVATE (object);

  switch (property_id)
    {
    case PROP_FAVORITE:
      g_value_set_boolean (value, priv->favorite);
      break;

    case PROP_ICON_FACTORY:
      g_value_set_object (value, priv->icon_factory);
      break;

    case PROP_NOTIFICATIONS:
      g_value_set_object (value, priv->note);

    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    case PROP_ROW:
      g_value_set_boxed (value, priv->row);
      break;

    case PROP_ACTIVE:
      g_value_set_boolean (value, priv->active);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_service_item_set_drag_state (CarrickServiceItem *self)
{
  if (self->priv->favorite)
    {
      /* Define the service as a drag source/destination */
      gtk_drag_source_set (GTK_WIDGET (self),
                           GDK_BUTTON1_MASK,
                           carrick_targets,
                           G_N_ELEMENTS (carrick_targets),
                           GDK_ACTION_MOVE);
      gtk_drag_dest_set (GTK_WIDGET (self),
                         GTK_DEST_DEFAULT_ALL,
                         carrick_targets,
                         G_N_ELEMENTS (carrick_targets),
                         GDK_ACTION_MOVE);
    }
  else
    {
      gtk_drag_source_unset (GTK_WIDGET (self));
      gtk_drag_dest_unset (GTK_WIDGET (self));
    }
}

static void
_service_item_set_security (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;
  gchar                     *security = g_strdup (priv->security);
  gchar                     *security_label = NULL;

  if (security && security[0] != '\0' && g_strcmp0 ("none", security) != 0)
    {
      if ((g_strcmp0 ("rsn", security) == 0) ||
	  (g_strcmp0 ("psk", security) == 0))
        {
          g_free (security);
          security = g_strdup ("WPA");
        }
      else
        {
          gint i;

          for (i = 0; security[i] != '\0'; i++)
            {
              security[i] = g_ascii_toupper (security[i]);
            }
        }
      /* TRANSLATORS: this is a wireless security method, at least WEP,
       *  WPA and IEEE8021X are possible token values. Example: "WEP encrypted".
       */
      security_label = g_strdup_printf (_ ("%s encrypted"),
                                        security);
    }
  else
    {
      security_label = g_strdup ("");
    }

  /* just in case security_sample has not been translated in _init ... */
  if (strlen (security_label) >
      gtk_label_get_width_chars (GTK_LABEL (priv->security_label)))
    {
      gtk_label_set_width_chars (GTK_LABEL (priv->security_label), -1);
    }

  gtk_label_set_text (GTK_LABEL (priv->security_label),
                      security_label);
  g_free (security_label);
  g_free (security);
}

static void
_populate_variables (CarrickServiceItem *self)
{
  CarrickServiceItemPrivate *priv = self->priv;
  GtkTreeIter                iter;

  g_return_if_fail (priv->model != NULL);
  g_return_if_fail (CARRICK_IS_NETWORK_MODEL (priv->model));

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->model),
                               &iter,
                               gtk_tree_row_reference_get_path (priv->row)))
    {
      char *method, *address, *netmask, *gateway;
      char **nameservers;
      char *state;

      if (priv->proxy)
        g_object_unref (priv->proxy);
      g_free (priv->name);
      g_free (priv->type);
      g_free (priv->security);
      g_free (priv->passphrase);
      g_free (priv->method);
      g_free (priv->address);
      g_free (priv->netmask);
      g_free (priv->gateway);
      g_strfreev (priv->nameservers);

      gtk_tree_model_get (GTK_TREE_MODEL (priv->model), &iter,
                          CARRICK_COLUMN_PROXY, &priv->proxy,
                          CARRICK_COLUMN_INDEX, &priv->index,
                          CARRICK_COLUMN_NAME, &priv->name,
                          CARRICK_COLUMN_TYPE, &priv->type,
                          CARRICK_COLUMN_STATE, &state,
                          CARRICK_COLUMN_STRENGTH, &priv->strength,
                          CARRICK_COLUMN_SECURITY, &priv->security,
                          CARRICK_COLUMN_PASSPHRASE_REQUIRED, &priv->need_pass,
                          CARRICK_COLUMN_PASSPHRASE, &priv->passphrase,
                          CARRICK_COLUMN_SETUP_REQUIRED, &priv->setup_required,
                          CARRICK_COLUMN_METHOD, &method,
                          CARRICK_COLUMN_ADDRESS, &address,
                          CARRICK_COLUMN_NETMASK, &netmask,
                          CARRICK_COLUMN_GATEWAY, &gateway,
                          CARRICK_COLUMN_CONFIGURED_METHOD, &priv->method,
                          CARRICK_COLUMN_CONFIGURED_ADDRESS, &priv->address,
                          CARRICK_COLUMN_CONFIGURED_NETMASK, &priv->netmask,
                          CARRICK_COLUMN_CONFIGURED_GATEWAY, &priv->gateway,
                          CARRICK_COLUMN_NAMESERVERS, &nameservers,
                          CARRICK_COLUMN_CONFIGURED_NAMESERVERS, &priv->nameservers,
                          CARRICK_COLUMN_FAVORITE, &priv->favorite,
                          CARRICK_COLUMN_IMMUTABLE, &priv->immutable,
                          CARRICK_COLUMN_LOGIN_REQUIRED, &priv->login_required,
                          -1);

      /* use normal values only if manually configured values are not available:
       * Two reasons:
       * 1. sometimes normal values are not available (e.g. when not connected).
       * 2. don't show "old" values while connman is still applying manually set ones
       */
      if (priv->method)
        g_free (method);
      else
        priv->method = method;
      if (priv->address)
        g_free (address);
      else
        priv->address = address;
      if (priv->netmask)
        g_free (netmask);
      else
        priv->netmask = netmask;
      if (priv->gateway)
        g_free (gateway);
      else
        priv->gateway = gateway;
      if (priv->nameservers)
        g_free (nameservers);
      else
        priv->nameservers = nameservers;

      /* TODO: it would make sense to expand this to cover lot of other 
       * state transitions so populate_variables() did not have to do so 
       * much unnecessary work */
      if (priv->state &&
          g_strcmp0 (priv->state, "failure") != 0 &&
          g_strcmp0 (state, "failure") == 0)
        priv->error_hidden = FALSE;
      g_free (priv->state);
      priv->state = state;
    }
}

static void
_set_form_modified (CarrickServiceItem *item, gboolean modified)
{
  item->priv->form_modified = modified;
  gtk_widget_set_sensitive (item->priv->apply_button, modified);
  if (modified) 
    gtk_widget_hide (item->priv->info_bar);
}

static void
_set_form_state (CarrickServiceItem *self)
{
  CarrickServiceItemPrivate *priv = self->priv;
  gchar                     *nameservers = NULL;
  GtkTextBuffer             *buf;
  GtkComboBox               *combo;

  combo = GTK_COMBO_BOX (priv->method_combo);

  /* This is a bit of a hack: we don't want to show "Fixed" as 
   * option if it's not sent by connman */
  if (g_strcmp0 (priv->method, "manual") == 0)
    {
      if (gtk_combo_box_get_active (combo) == METHOD_FIXED)
        gtk_combo_box_remove_text (combo, METHOD_FIXED);

      gtk_combo_box_set_active (combo, METHOD_MANUAL);
      gtk_widget_set_sensitive (GTK_WIDGET (combo), TRUE);
    }
  else if (g_strcmp0 (priv->method, "dhcp") == 0)
    {
      if (gtk_combo_box_get_active (combo) == METHOD_FIXED)
        gtk_combo_box_remove_text (combo, METHOD_FIXED);

      gtk_combo_box_set_active (combo, METHOD_DHCP);
      gtk_widget_set_sensitive (GTK_WIDGET (combo), TRUE);
    }
  else if (g_strcmp0 (priv->method, "fixed") == 0)
    {
      if (gtk_combo_box_get_active (combo) != METHOD_FIXED)
        {
           /* TRANSLATORS: This string will be in the "Connect by:"-
            * combobox just like "DHCP" and "Static IP". Fixed means
            * that the IP configuration cannot be changed at all,
            * like in a 3G network */
          gtk_combo_box_insert_text (combo, METHOD_FIXED, _("Fixed IP"));
        }
      gtk_combo_box_set_active (combo, METHOD_FIXED);
      gtk_widget_set_sensitive (GTK_WIDGET (combo), FALSE);
    }
  else if (priv->method)
    {
      g_warning ("Unknown service ipv4 method '%s'", priv->method);
    }

  gtk_entry_set_text (GTK_ENTRY (priv->address_entry),
                      priv->address ? priv->address : "");

  gtk_entry_set_text (GTK_ENTRY (priv->netmask_entry),
                      priv->netmask ? priv->netmask : "");

  gtk_entry_set_text (GTK_ENTRY (priv->gateway_entry),
                      priv->gateway ? priv->gateway : "");

  buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->dns_text_view));
  if (priv->nameservers)
    {
      nameservers = g_strjoinv ("\n", priv->nameservers);
      gtk_text_buffer_set_text (buf, nameservers, -1);
      g_free (nameservers);
    }
  else
    {
      gtk_text_buffer_set_text (buf, "", -1);
    }

  /* need to initialize this after setting the widgets, as their signal
   * handlers set form_modified to TRUE */
  _set_form_modified (self, FALSE);
}

static void
_set_state (CarrickServiceItem *self)
{
  CarrickServiceItemPrivate *priv = self->priv;
  gchar                     *label = NULL;
  gchar                     *button = NULL;
  GdkPixbuf                 *pixbuf = NULL;
  gchar                     *name = g_strdup (priv->name);

  _service_item_set_security (self);
  _service_item_set_drag_state (self);

  gtk_widget_set_sensitive (priv->advanced_box, !priv->immutable);
  gtk_widget_hide (priv->portal_button);

  if (g_strcmp0 ("ethernet", priv->type) == 0)
    {
      g_free (name);
      name = g_strdup (_ ("Wired"));
    }

  if (g_strcmp0 (priv->state, "ready") == 0)
    {
      if (g_strcmp0 (priv->type, "ethernet") != 0 &&
          g_strcmp0 (priv->type, "vpn") != 0)
        {
          /* Only expose delete button for non-ethernet, non-vpn devices */
          gtk_widget_show (GTK_WIDGET (priv->delete_button));
          gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                                    TRUE);
        }
      button = g_strdup (_ ("Disconnect"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Connected"));
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
      gtk_widget_set_visible (priv->portal_button, priv->login_required);
    }
  else if (g_strcmp0 (priv->state, "online") == 0)
    {
      if (g_strcmp0 (priv->type, "ethernet") != 0 &&
          g_strcmp0 (priv->type, "vpn") != 0)
        {
          /* Only expose delete button for non-ethernet, non-vpn devices */
          gtk_widget_show (GTK_WIDGET (priv->delete_button));
          gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                                    TRUE);
        }
      button = g_strdup (_ ("Disconnect"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Online"));
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else if (g_strcmp0 (priv->state, "configuration") == 0)
    {
      button = g_strdup_printf (_ ("Cancel"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Configuring"));
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else if (g_strcmp0 (priv->state, "association") == 0)
    {
      button = g_strdup_printf (_("Cancel"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _("Associating"));
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else if (g_strcmp0 (priv->state, "idle") == 0)
    {
      gtk_widget_hide (GTK_WIDGET (priv->delete_button));
      gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                                FALSE);
      button = g_strdup (_ ("Connect"));
      label = g_strdup (name);
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else if (g_strcmp0 (priv->state, "failure") == 0)
    {
      /*
       * If the connection failed we should allow the user
       * to remove the connection (and forget the password.
       * Except, of course, when the connection is Ethernet or VPN.
       */
      if (g_strcmp0 (priv->type, "ethernet") != 0 &&
          g_strcmp0 (priv->type, "vpn") != 0)
        {
          gtk_widget_hide (GTK_WIDGET (priv->delete_button));
          gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                                    FALSE);
        }
      else
        {
          gtk_widget_show (GTK_WIDGET (priv->delete_button));
          gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                                    TRUE);
        }

      button = g_strdup (_ ("Connect"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Connection failed"));
      if (!priv->error_hidden)
        {
          gtk_label_set_text (GTK_LABEL (priv->info_label),
                              _("Sorry, the connection failed. You could try again."));
          gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                         GTK_MESSAGE_ERROR);
        }
    }
  else if (g_strcmp0 (priv->state, "disconnect") == 0)
    {
      button = g_strdup_printf (_ ("Disconnecting"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Disconnecting"));
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else
    {
      label = g_strdup (name);
    }

  if (label && label[0] != '\0')
    {
      gtk_label_set_text (GTK_LABEL (priv->name_label),
                          label);
      pixbuf = carrick_icon_factory_get_pixbuf_for_state (
                  priv->icon_factory,
                  carrick_icon_factory_get_state (self->priv->type,
                                                  self->priv->strength));

      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon),
                                 pixbuf);
    }

  if (button && button[0] != '\0')
    gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                          button);

  if (g_str_equal (gtk_label_get_text (GTK_LABEL (priv->info_label)), ""))
    gtk_widget_hide (priv->info_bar);
  else
    gtk_widget_show (priv->info_bar);

  /* only update the entries etc if the user hasn't touched them yet */
  if (!priv->form_modified)
    {
      _set_form_state (self);
    }

  g_free (name);
  g_free (label);
  g_free (button);
}

static void
_show_pass_toggled_cb (GtkToggleButton    *button,
                       CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  if (!priv->passphrase_hint_visible)
    {
      gboolean vis = gtk_toggle_button_get_active (button);
      gtk_entry_set_visibility (GTK_ENTRY (priv->passphrase_entry),
                                vis);
    }

  carrick_service_item_set_active (item, TRUE);
}

static void
_request_passphrase (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  if (!priv->passphrase || (strlen (priv->passphrase) == 0))
    {
      /* TRANSLATORS: text should be 20 characters or less to be entirely
       * visible in the passphrase entry */
      gtk_entry_set_text (GTK_ENTRY (priv->passphrase_entry),
                          _ ("Type password here"));
      priv->passphrase_hint_visible = TRUE;
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (priv->passphrase_entry),
                          priv->passphrase);
    }    

  gtk_entry_set_visibility (GTK_ENTRY (priv->passphrase_entry), TRUE);
  gtk_editable_select_region (GTK_EDITABLE (priv->passphrase_entry),
                              0, -1);
  gtk_widget_grab_focus (priv->passphrase_entry);
  gtk_widget_hide (priv->connect_box);
  gtk_widget_show (priv->passphrase_box);
}

/*
 * Generic call_notify function for async d-bus calls
 */
static void
dbus_proxy_notify_cb (DBusGProxy *proxy,
                      GError     *error,
                      gpointer    user_data)
{
  if (error)
    {
      g_warning ("Error when ending call: %s",
                 error->message);
      g_error_free (error);
    }
}

static void
_connect (CarrickServiceItem *self)
{
    /* Set an unusually long timeout because the Connect
     * method doesn't return until there has been either
     * success or an error.
     */
    dbus_g_proxy_set_default_timeout (self->priv->proxy, 120000);
    net_connman_Service_connect_async (self->priv->proxy,
                                       dbus_proxy_notify_cb,
                                       self);
    dbus_g_proxy_set_default_timeout (self->priv->proxy, -1);
}

static void
set_passphrase_notify_cb (DBusGProxy *proxy,
                          GError     *error,
                          gpointer    user_data)
{
  CarrickServiceItem *item = CARRICK_SERVICE_ITEM (user_data);

  if (error)
    {
      g_warning ("Error when ending call: %s",
                 error->message);
      g_error_free (error);
    }
  else
    {
      _connect (item);
    }
}

static void
_delete_button_cb (GtkButton *delete_button,
                   gpointer   user_data)
{
  CarrickServiceItem        *item = CARRICK_SERVICE_ITEM (user_data);
  CarrickServiceItemPrivate *priv = item->priv;
  GtkWidget                 *dialog;
  GtkWidget                 *label;
  gchar                     *label_text = NULL;

  dialog = gtk_dialog_new_with_buttons (_ ("Really remove?"),
                                        GTK_WINDOW (gtk_widget_get_toplevel (user_data)),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_REJECT,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);

  carrick_shell_close_dialog_on_hide (GTK_DIALOG (dialog));

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog),
                                FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   GTK_RESPONSE_ACCEPT);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_icon_name (GTK_WINDOW (dialog),
                            GTK_STOCK_DELETE);

  label_text = g_strdup_printf (_ ("Do you want to remove the %s %s network? "
                                   "This\nwill forget the password and you will"
                                   " no longer be\nautomatically connected to "
                                   "%s."),
                                priv->name,
                                util_get_service_type_for_display (priv->type),
                                priv->name);
  label = gtk_label_new (label_text);

  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                       12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      label,
                      TRUE,
                      TRUE,
                      6);
  gtk_widget_show_all (dialog);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      carrick_service_item_set_active (item, FALSE);
      if (g_strcmp0 (priv->state, "failure") == 0)
        {
          /* The service is marked failure, use Service.Remove to set
           * back to idle state and clear password and favorite flag
           */
          net_connman_Service_remove_async (priv->proxy,
                                            dbus_proxy_notify_cb,
                                            item);
        }
      else if (priv->favorite)
        {
          /* If the network is a favorite it's set to auto-connect and
           * has therefore been connected before, use Service.Remove
           */
          carrick_notification_manager_queue_event (priv->note,
                                                    priv->type,
                                                    "idle",
                                                    priv->name);
          net_connman_Service_remove_async (priv->proxy,
                                            dbus_proxy_notify_cb,
                                            item);
        }
      else
        {
          /* The service isn't marked favorite so probably hasn't been
           * successfully connected to before, just clear the passphrase
           */
          net_connman_Service_clear_property_async (priv->proxy,
                                                    "Passphrase",
                                                    dbus_proxy_notify_cb,
                                                    item);
        }
    }

  /*
   * Now explicitely focus the panel (MB#7304)
   */
  carrick_shell_request_focus ();

  gtk_widget_destroy (dialog);
}

static void
_start_connecting (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  if (priv->security && g_str_equal (priv->security, "none") == FALSE)
    {
      /* ask for a passphrase for non-immutable services that 
       * require a password or failed to connect last time */
      if (!priv->immutable &&
          ((g_strcmp0 (priv->state, "failure") == 0) ||
           (priv->need_pass && priv->passphrase == NULL)))
        {
          _request_passphrase (item);
        }
      else
        {
          carrick_notification_manager_queue_event (priv->note,
                                                    priv->type,
                                                    "ready",
                                                    priv->name);
          _connect (item);
        }
    }
  else
    {
      if (priv->setup_required)
        {
          GPid pid;
          gulong flags = G_SPAWN_SEARCH_PATH | \
                         G_SPAWN_STDOUT_TO_DEV_NULL | \
                         G_SPAWN_STDERR_TO_DEV_NULL;
          GError *error = NULL;
          char *argv[] = {
              LIBEXECDIR "/carrick-3g-wizard",
              NULL
          };

          if (g_spawn_async(NULL, argv, NULL, flags, NULL,
                            NULL, &pid, &error))
            {
              carrick_shell_hide ();
            } else {
              g_warning ("Unable to spawn 3g connection wizard");
              g_error_free(error);
            }
        }
      else
        {
          carrick_notification_manager_queue_event (priv->note,
                                                    priv->type,
                                                    "ready",
                                                    priv->name);
          _connect (item);
        }
    }
}

static void
_connect_button_cb (GtkButton          *connect_button,
                    CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  if (g_str_equal (priv->state, "online") ||
      g_str_equal (priv->state, "ready") ||
      g_str_equal (priv->state, "configuration") ||
      g_str_equal (priv->state, "association"))
    {
      carrick_notification_manager_queue_event (priv->note,
                                                priv->type,
                                                "idle",
                                                priv->name);
      carrick_service_item_set_active (item, FALSE);
      net_connman_Service_disconnect_async (priv->proxy,
                                            dbus_proxy_notify_cb,
                                            item);
    }
  else
    {
      carrick_service_item_set_active (item, TRUE);
      _start_connecting (item);
    }
}

static void
_portal_button_cb (GtkButton          *button,
                   CarrickServiceItem *item)
{
   if (g_app_info_launch_default_for_uri ("http://connman.net", NULL, NULL))
     carrick_shell_hide ();
}

static void
_advanced_expander_notify_expanded_cb (GObject    *object,
                                       GParamSpec *param_spec,
                                       gpointer    data)
{
  CarrickServiceItemPrivate *priv;
  gboolean expanded;

  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (data));
  priv = CARRICK_SERVICE_ITEM (data)->priv;

  carrick_service_item_set_active (CARRICK_SERVICE_ITEM (data), TRUE);

  expanded = gtk_expander_get_expanded (GTK_EXPANDER (priv->advanced_expander));
  if (expanded)
    {
      /* update user changed values with connman data */
      priv->form_modified = FALSE;
      _set_state (CARRICK_SERVICE_ITEM (data));

      gtk_widget_hide (priv->info_bar);
    }
  nbtk_gtk_expander_set_expanded (NBTK_GTK_EXPANDER (priv->expando),
                                  expanded);
}

static void
_connect_with_password (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;
  const gchar               *passphrase;
  GValue                    *value;
  gchar                     *label = NULL;

  carrick_notification_manager_queue_event (priv->note,
                                            priv->type,
                                            "ready",
                                            priv->name);

  if (priv->passphrase_hint_visible)
    {
      passphrase = "";
    }
  else
    {
      passphrase = gtk_entry_get_text (GTK_ENTRY (priv->passphrase_entry));
    
      if (util_validate_wlan_passphrase (priv->security,
                                         passphrase,
                                         &label))
        {
          GtkTreeIter iter;
        
          value = g_slice_new0 (GValue);
          g_value_init (value, G_TYPE_STRING);
          g_value_set_string (value, passphrase);

          net_connman_Service_set_property_async (priv->proxy,
                                                  "Passphrase",
                                                  value,
                                                  set_passphrase_notify_cb,
                                                  item);

          g_value_unset (value);
          g_slice_free (GValue, value);

          gtk_widget_hide (priv->passphrase_box);
          gtk_widget_show (priv->connect_box);

          /* hack: modify passphrase here since connman won't
           * send property change notify */
          gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->model),
                                   &iter, gtk_tree_row_reference_get_path (priv->row));
          gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter,
                              CARRICK_COLUMN_PASSPHRASE,
                              passphrase,
                              -1);
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (priv->info_label),
                              label);
          gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                         GTK_MESSAGE_WARNING);
          gtk_widget_show (priv->info_bar);
          gtk_widget_grab_focus (priv->passphrase_entry);
        }
    }
}

static void
_connect_with_pw_clicked_cb (GtkButton *btn, CarrickServiceItem *item)
{
  carrick_service_item_set_active (item, TRUE);
  _connect_with_password (item);
}

static void
_passphrase_entry_activated_cb (GtkEntry *entry, CarrickServiceItem *item)
{
  carrick_service_item_set_active (item, TRUE);
  _connect_with_password (item);
}

static void
_entry_changed_cb (GtkEntry *pw_entry, CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  if (priv->passphrase_hint_visible)
    {
      gboolean visible = gtk_toggle_button_get_active (
        GTK_TOGGLE_BUTTON (priv->show_password_check));

      gtk_entry_set_visibility (pw_entry, visible);
      priv->passphrase_hint_visible = FALSE;
    }
}

static void
_passphrase_entry_clear_released_cb (GtkEntry            *entry,
                                     GtkEntryIconPosition icon_pos,
                                     GdkEvent            *event,
                                     gpointer             user_data)
{
  CarrickServiceItem *self = CARRICK_SERVICE_ITEM (user_data);
  CarrickServiceItemPrivate *priv = self->priv;

  if (gtk_entry_get_text_length (entry) > 0)
    {
      gtk_entry_set_text (entry, "");
      gtk_widget_grab_focus (GTK_WIDGET (entry));
    }
  /* On the second click of the clear button hide the passphrase widget */
  else
    {
      gtk_widget_hide (priv->passphrase_box);
      gtk_widget_hide (priv->info_bar);
      gtk_label_set_text (GTK_LABEL (priv->info_label), "");
    }
}

gboolean
carrick_service_item_get_favorite (CarrickServiceItem *item)
{
  g_return_val_if_fail (CARRICK_IS_SERVICE_ITEM (item), FALSE);

  return item->priv->favorite;
}

static void
_unexpand_advanced_settings (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  g_signal_handlers_block_by_func (priv->advanced_expander,
                                   _advanced_expander_notify_expanded_cb,
                                   item);
  gtk_expander_set_expanded (GTK_EXPANDER (priv->advanced_expander),
                             FALSE);
  g_signal_handlers_unblock_by_func (priv->advanced_expander,
                                     _advanced_expander_notify_expanded_cb,
                                     item);

  nbtk_gtk_expander_set_expanded (NBTK_GTK_EXPANDER (priv->expando),
                                  FALSE);
}

gboolean
carrick_service_item_get_active (CarrickServiceItem *item)
{
  g_return_val_if_fail (CARRICK_IS_SERVICE_ITEM (item), FALSE);

  return item->priv->active;
}

void
carrick_service_item_set_active (CarrickServiceItem *item,
                                 gboolean            active)
{
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (item));

  CarrickServiceItemPrivate *priv = item->priv;

  if (priv->active == active)
    return;

  priv->active = active;
  if (!active)
    {
      priv->error_hidden = TRUE;
      gtk_widget_hide (priv->passphrase_box);
      gtk_widget_show (priv->connect_box);
      gtk_widget_hide (priv->info_bar);
      gtk_label_set_text (GTK_LABEL (priv->info_label), "");
      _unexpand_advanced_settings (item);

      /* overwrite any form changes user made */
      _set_form_state (item);
    }
  g_object_notify (G_OBJECT (item), "active");
}

DBusGProxy*
carrick_service_item_get_proxy (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  return priv->proxy;
}

GtkTreeRowReference*
carrick_service_item_get_row_reference (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  return priv->row;
}


gint
carrick_service_item_get_order (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  return priv->index;
}

static void
_set_model (CarrickServiceItem  *self,
            CarrickNetworkModel *model)
{
  g_return_if_fail (model != NULL);
  CarrickServiceItemPrivate *priv = self->priv;

  priv->model = model;


  if (priv->row)
    {
      _populate_variables (self);
      _set_state (self);
    }
}

static void
_set_row (CarrickServiceItem *self,
          GtkTreePath        *path)
{
  g_return_if_fail (path != NULL);
  CarrickServiceItemPrivate *priv = self->priv;

  if (priv->row)
    {
      gtk_tree_row_reference_free (priv->row);
      priv->row = NULL;
    }

  if (path)
    priv->row = gtk_tree_row_reference_new (GTK_TREE_MODEL (priv->model),
                                            path);

  if (priv->model)
    {
      _populate_variables (self);
      _set_state (self);
    }
}

void
carrick_service_item_update (CarrickServiceItem *self)
{
  _populate_variables (self);
  _set_state (self);
}

static void
carrick_service_item_set_property (GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (object));
  CarrickServiceItem        *self = CARRICK_SERVICE_ITEM (object);
  CarrickServiceItemPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_ICON_FACTORY:
      priv->icon_factory = CARRICK_ICON_FACTORY (g_value_get_object (value));
      break;

    case PROP_NOTIFICATIONS:
      priv->note = CARRICK_NOTIFICATION_MANAGER (g_value_get_object (value));
      break;

    case PROP_MODEL:
      _set_model (self,
                  CARRICK_NETWORK_MODEL (g_value_get_object (value)));
      break;

    case PROP_ROW:
      _set_row (self,
                (GtkTreePath *) g_value_get_boxed (value));
      break;

    case PROP_ACTIVE:
      carrick_service_item_set_active (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_service_item_dispose (GObject *object)
{
  CarrickServiceItem        *item = CARRICK_SERVICE_ITEM (object);
  CarrickServiceItemPrivate *priv = item->priv;

  if (priv->row)
    {
      gtk_tree_row_reference_free (priv->row);
      priv->row = NULL;
    }

  if (priv->hand_cursor)
    {
      gdk_cursor_unref (priv->hand_cursor);
      priv->hand_cursor = NULL;
    }

  if (priv->proxy)
    {
      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }

  g_free (priv->name);
  priv->name = NULL;
  g_free (priv->type);
  priv->type = NULL;
  g_free (priv->security);
  priv->security = NULL;
  g_free (priv->passphrase);
  priv->passphrase = NULL;
  g_free (priv->method);
  priv->method = NULL;
  g_free (priv->address);
  priv->address = NULL;
  g_free (priv->netmask);
  priv->netmask = NULL;
  g_free (priv->gateway);
  priv->gateway = NULL;
  g_strfreev (priv->nameservers);
  priv->nameservers = NULL;

  G_OBJECT_CLASS (carrick_service_item_parent_class)->dispose (object);
}

static void
carrick_service_item_finalize (GObject *object)
{
  CarrickServiceItem        *item = CARRICK_SERVICE_ITEM (object);
  CarrickServiceItemPrivate *priv = item->priv;

  g_free (priv->name);
  g_free (priv->type);
  g_free (priv->state);
  g_free (priv->security);
  g_free (priv->passphrase);

  G_OBJECT_CLASS (carrick_service_item_parent_class)->finalize (object);
}

static gboolean
carrick_service_item_enter_notify_event (GtkWidget        *widget,
                                         GdkEventCrossing *event)
{
  CarrickServiceItem        *item = CARRICK_SERVICE_ITEM (widget);
  CarrickServiceItemPrivate *priv = item->priv;

  if (priv->favorite)
    gdk_window_set_cursor (widget->window, priv->hand_cursor);
  else
    gdk_window_set_cursor (widget->window, NULL);
  
  return TRUE;
}

static gboolean
carrick_service_item_leave_notify_event (GtkWidget        *widget,
                                         GdkEventCrossing *event)
{
  /* if pointer moves to a child widget, we want to keep the
   * ServiceItem prelighted, but show the normal cursor */
  if (event->detail == GDK_NOTIFY_INFERIOR ||
      event->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL)
    {
      gdk_window_set_cursor (widget->window, NULL);
    }

  return TRUE;
}

static void
method_combo_changed_cb (GtkComboBox *combobox,
                         gpointer     user_data)
{
  CarrickServiceItemPrivate *priv;
  gboolean manual;

  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));
  priv = CARRICK_SERVICE_ITEM (user_data)->priv;

  manual = (gtk_combo_box_get_active (combobox) == METHOD_MANUAL);

  gtk_widget_set_sensitive (priv->address_entry, manual);
  gtk_widget_set_sensitive (priv->netmask_entry, manual);
  gtk_widget_set_sensitive (priv->gateway_entry, manual);

  if (!manual)
    {
      GtkTextBuffer *buf;

      /* revert to last received data */
      gtk_entry_set_text (GTK_ENTRY (priv->address_entry),
                          priv->address ? priv->address : "");

      gtk_entry_set_text (GTK_ENTRY (priv->netmask_entry),
                          priv->netmask ? priv->netmask : "");

      gtk_entry_set_text (GTK_ENTRY (priv->gateway_entry),
                          priv->gateway ? priv->gateway : "");

      buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->dns_text_view));

      if (priv->nameservers)
        {
          char *nameservers;

          nameservers = g_strjoinv ("\n", priv->nameservers);
          gtk_text_buffer_set_text (buf, nameservers, -1);
          g_free (nameservers);
        }
      else
        {
          gtk_text_buffer_set_text (buf, "", -1);
        }
    }

  _set_form_modified (CARRICK_SERVICE_ITEM (user_data), TRUE);
}

static void
set_nameservers_configuration_cb (DBusGProxy *proxy,
                                  GError     *error,
                                  gpointer    user_data)
{
  CarrickServiceItemPrivate *priv;

  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));
  priv = CARRICK_SERVICE_ITEM (user_data)->priv;

  if (error)
    {
      /* TODO: errors we should show in UI? */

      g_warning ("Error setting Nameservers.Configuration: %s",
                 error->message);
      g_error_free (error);
    }

  if (g_str_equal (priv->state, "idle") ||
      g_str_equal (priv->state, "disconnect"))
    {
      _start_connecting (CARRICK_SERVICE_ITEM (user_data));
    }
}

static void
set_ipv4_configuration_cb (DBusGProxy *proxy,
                           GError     *error,
                           gpointer    user_data)

{
  CarrickServiceItemPrivate *priv;

  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));
  priv = CARRICK_SERVICE_ITEM (user_data)->priv;

  if (error)
    {
      /* TODO: errors we should show in UI? */

      g_warning ("Error setting IPv4 configuration: %s",
                 error->message);
      g_error_free (error);
      return;
    }

  /* Nothing to do here, set_nameservers_configuration_cb() will
   * re-connect if needed */
}

static gboolean
validate_address (const char *address, CarrickInet inet, gboolean allow_empty)
{
  unsigned char buf_v4[sizeof (struct in_addr)];
  unsigned char buf_v6[sizeof(struct in6_addr)];

  if (!address || strlen (address) == 0)
    return allow_empty;

  switch (inet)
    {
    case CARRICK_INET_V4:
      return (inet_pton (AF_INET, address, buf_v4) == 1);
    case CARRICK_INET_V6:
      return (inet_pton (AF_INET6, address, buf_v6) == 1);
    case CARRICK_INET_ANY:
      return (inet_pton (AF_INET, address, buf_v4) == 1 ||
              inet_pton (AF_INET6, address, buf_v6) == 1);
    default:
      g_warn_if_reached ();
      return FALSE;
    }
}

static gboolean
validate_static_ipv4_entries (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv;
  const char *address;

  priv = item->priv;
  address = gtk_entry_get_text (GTK_ENTRY (priv->address_entry));
  if (!validate_address (address, CARRICK_INET_V4, FALSE))
    {
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          _("Sorry, it looks like the IP address is not valid"));
      gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                     GTK_MESSAGE_WARNING);
      gtk_widget_show (priv->info_bar);
      gtk_widget_grab_focus (priv->address_entry);
      return FALSE;
    }

  address = gtk_entry_get_text (GTK_ENTRY (priv->gateway_entry));
  if (!validate_address (address, CARRICK_INET_V4, TRUE))
    {
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          _("Sorry, it looks like the gateway address is not valid"));
      gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                     GTK_MESSAGE_WARNING);
      gtk_widget_show (priv->info_bar);
      gtk_widget_grab_focus (priv->gateway_entry);
      return FALSE;
    }

  address = gtk_entry_get_text (GTK_ENTRY (priv->netmask_entry));
  if (!validate_address (address, CARRICK_INET_V4, TRUE))
    {
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          _("Sorry, it looks like the subnet mask is not valid"));
      gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                     GTK_MESSAGE_WARNING);
      gtk_widget_show (priv->info_bar);
      gtk_widget_grab_focus (priv->netmask_entry);
      return FALSE;
    }

  return TRUE;
}

static char**
get_nameserver_strv (GtkTextView *dns_text_view)
{
  GtkTextBuffer *buf;
  GtkTextIter start, end;
  char *nameservers;
  char **dnsv;

  buf = gtk_text_view_get_buffer (dns_text_view);
  gtk_text_buffer_get_start_iter (buf, &start);
  gtk_text_buffer_get_end_iter (buf, &end);
  nameservers = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
  dnsv = g_strsplit_set (nameservers, " ,;\n", -1);

  g_free (nameservers);
  return dnsv;
}

static gboolean
validate_dns_text_view (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv;
  char **dnsv, **iter;

  priv = item->priv;

  dnsv = get_nameserver_strv (GTK_TEXT_VIEW (priv->dns_text_view));

  for (iter = dnsv; *iter; iter++)
    {
      if (!validate_address (*iter, CARRICK_INET_ANY, TRUE))
        {
          char *msg;

          msg = g_strdup_printf (_("Sorry, it looks like the nameserver address '%s' is not valid"),
                                 *iter);
          gtk_label_set_text (GTK_LABEL (priv->info_label),
                              msg);
          g_free (msg);
          gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                         GTK_MESSAGE_WARNING);
          gtk_widget_show (priv->info_bar);
          gtk_widget_grab_focus (priv->dns_text_view);

          g_strfreev (dnsv);
          return FALSE;
        }
    }

  g_strfreev (dnsv);

  return TRUE;
}

static void
static_ip_entry_notify_text (GtkEntry   *entry,
                             GParamSpec *pspec,
                             gpointer    user_data)
{
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));

  _set_form_modified (CARRICK_SERVICE_ITEM (user_data), TRUE);
}

static void
dns_buffer_changed_cb (GtkTextBuffer *textbuffer,
                       gpointer       user_data)
{
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));

  _set_form_modified (CARRICK_SERVICE_ITEM (user_data), TRUE);
}

static void
apply_button_clicked_cb (GtkButton *button,
                         gpointer   user_data)
{
  CarrickServiceItem *item;
  CarrickServiceItemPrivate *priv;
  GValue *value;
  char **dnsv;
  int method;

  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));
  item = CARRICK_SERVICE_ITEM (user_data);
  priv = item->priv;

  carrick_service_item_set_active (item, TRUE);

  if (!validate_dns_text_view (item))
    return;

  method = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->method_combo));
  if (method != METHOD_FIXED)
    {
      GHashTable *ipv4;

      /* save ipv4 settings */

      ipv4 = g_hash_table_new (g_str_hash, g_str_equal);
      if (method == METHOD_MANUAL)
        {
          char *address, *netmask, *gateway;

          if (!validate_static_ipv4_entries (item))
            {
               g_hash_table_destroy (ipv4);
               return;
            }

          address = (char*)gtk_entry_get_text (GTK_ENTRY (priv->address_entry));
          netmask = (char*)gtk_entry_get_text (GTK_ENTRY (priv->netmask_entry));
          gateway = (char*)gtk_entry_get_text (GTK_ENTRY (priv->gateway_entry));

          g_hash_table_insert (ipv4, "Method", "manual");
          g_hash_table_insert (ipv4, "Address", address);
          g_hash_table_insert (ipv4, "Netmask", netmask);
          g_hash_table_insert (ipv4, "Gateway", gateway);
        }
      else
        {
          g_hash_table_insert (ipv4, "Method", "dhcp");
        }

      value = g_new0 (GValue, 1);
      g_value_init (value, DBUS_TYPE_G_STRING_STRING_HASHTABLE);
      g_value_set_boxed (value, ipv4);

      net_connman_Service_set_property_async (priv->proxy,
                                              "IPv4.Configuration",
                                              value,
                                              set_ipv4_configuration_cb,
                                              user_data);  

      g_free (value);
      g_hash_table_destroy (ipv4);
    }

  /* start updating form again based on connman updates */
  _set_form_modified (item, FALSE);

  /* save dns settings */
  value = g_new0 (GValue, 1);
  g_value_init (value, G_TYPE_STRV);
  dnsv = get_nameserver_strv (GTK_TEXT_VIEW (priv->dns_text_view));
  g_value_set_boxed (value, dnsv);

  net_connman_Service_set_property_async (priv->proxy,
                                          "Nameservers.Configuration",
                                          value,
                                          set_nameservers_configuration_cb,
                                          user_data);

  g_free (value);
  g_strfreev (dnsv);

  _unexpand_advanced_settings (item);
}

static void
button_size_request_cb (GtkWidget      *button,
                        GtkRequisition *requisition,
                        gpointer        user_data)
{
  requisition->width = MAX (requisition->width, CARRICK_MIN_BUTTON_WIDTH);
}

static void
carrick_service_item_class_init (CarrickServiceItemClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (CarrickServiceItemPrivate));

  object_class->get_property = carrick_service_item_get_property;
  object_class->set_property = carrick_service_item_set_property;
  object_class->dispose = carrick_service_item_dispose;
  object_class->finalize = carrick_service_item_finalize;

  widget_class->enter_notify_event = carrick_service_item_enter_notify_event;
  widget_class->leave_notify_event = carrick_service_item_leave_notify_event;

  pspec = g_param_spec_boolean ("favorite",
                                "Favorite",
                                "Is the service a favorite (i.e. used previously)",
                                FALSE,
                                G_PARAM_READABLE);
  g_object_class_install_property (object_class,
                                   PROP_FAVORITE,
                                   pspec);

  pspec = g_param_spec_object ("notification-manager",
                               "CarrickNotificationManager",
                               "Notification manager to use",
                               CARRICK_TYPE_NOTIFICATION_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_NOTIFICATIONS,
                                   pspec);

  pspec = g_param_spec_object ("icon-factory",
                               "icon-factory",
                               "CarrickIconFactory object",
                               CARRICK_TYPE_ICON_FACTORY,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_ICON_FACTORY,
                                   pspec);

  pspec = g_param_spec_object ("model",
                               "model",
                               "CarrickNetworkModel object",
                               CARRICK_TYPE_NETWORK_MODEL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   pspec);

  pspec = g_param_spec_boxed ("tree-path",
                              "tree-path",
                              "GtkTreePath of the row to represent",
                              GTK_TYPE_TREE_PATH,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_ROW,
                                   pspec);

  pspec = g_param_spec_boolean ("active",
                                "active",
                                "Whether ServiceItem is considered the active UI element",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
                                   PROP_ACTIVE,
                                   pspec);
}

static GtkWidget*
add_label_to_table (GtkTable *table, guint row, const char *text)
{
  GtkWidget *label;

  label = gtk_label_new (text);
  gtk_widget_show (label);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_misc_set_padding (GTK_MISC (label), 0, 5);
  gtk_table_attach (table, label,
                    0, 1, row, row + 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);

  return label;
}

static GtkWidget*
add_entry_to_table (GtkTable *table, guint row)
{
  GtkWidget *entry;

  entry = gtk_entry_new_with_max_length (15);
  gtk_widget_show (entry);
  gtk_widget_set_size_request (entry, 100, -1);
  gtk_table_attach (table, entry,
                    1, 2, row, row + 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);

  return entry;
}

static void
carrick_service_item_init (CarrickServiceItem *self)
{
  CarrickServiceItemPrivate *priv;
  GtkWidget                 *box, *hbox, *vbox;
  GtkWidget                 *image;
  GtkWidget                 *table;
  GtkWidget                 *align;
  GtkWidget                 *scrolled_window;
  GtkWidget                 *connect_with_pw_button;
  GtkWidget                 *content_area;
  char                      *security_sample;

  priv = self->priv = SERVICE_ITEM_PRIVATE (self);

  priv->model = NULL;
  priv->row = NULL;

  priv->proxy = NULL;
  priv->index = 0;
  priv->name = NULL;
  priv->type = NULL;
  priv->state = NULL;
  priv->strength = 0;
  priv->security = NULL;
  priv->need_pass = FALSE;
  priv->passphrase = NULL;
  priv->icon_factory = NULL;
  priv->note = NULL;
  priv->setup_required = FALSE;
  priv->favorite = FALSE;
  priv->error_hidden = TRUE;

  priv->hand_cursor = gdk_cursor_new (GDK_HAND1);

  gtk_event_box_set_visible_window (GTK_EVENT_BOX (self), FALSE);
  
  box = gtk_hbox_new (FALSE,
                      6);
  gtk_widget_show (box);
  priv->expando = nbtk_gtk_expander_new ();
  gtk_container_add (GTK_CONTAINER (self),
                     priv->expando);
  nbtk_gtk_expander_set_label_widget (NBTK_GTK_EXPANDER (priv->expando),
                                      box);
  nbtk_gtk_expander_set_has_indicator (NBTK_GTK_EXPANDER (priv->expando),
                                       FALSE);
  gtk_widget_show (priv->expando);

  priv->icon = gtk_image_new ();
  gtk_widget_show (priv->icon);
  gtk_box_pack_start (GTK_BOX (box),
                      priv->icon,
                      FALSE,
                      FALSE,
                      6);

  vbox = gtk_vbox_new (FALSE,
                       6);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (box),
                      vbox,
                      TRUE,
                      TRUE,
                      6);

  hbox = gtk_hbox_new (FALSE,
                       6);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox,
                      TRUE,
                      TRUE,
                      6);

  priv->name_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (priv->name_label),
                          0.0, 0.5);
  gtk_widget_show (priv->name_label);
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->name_label,
                      TRUE,
                      TRUE,
                      6);

  image = gtk_image_new_from_icon_name ("edit-clear",
                                        GTK_ICON_SIZE_MENU);
  gtk_widget_show (image);
  priv->delete_button = gtk_button_new ();
  g_signal_connect (priv->delete_button,
                    "clicked",
                    G_CALLBACK (_delete_button_cb),
                    self);
  gtk_button_set_relief (GTK_BUTTON (priv->delete_button),
                         GTK_RELIEF_NONE);
  gtk_button_set_image (GTK_BUTTON (priv->delete_button),
                        image);
  gtk_box_pack_end (GTK_BOX (box),
                    priv->delete_button,
                    FALSE,
                    FALSE,
                    6);
  gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                            FALSE);

  priv->connect_box = gtk_hbox_new (FALSE,
                                    6);
  gtk_widget_show (priv->connect_box);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->connect_box,
                      TRUE,
                      TRUE,
                      6);

  align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_widget_show (align);
  gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      align,
                      FALSE,
                      FALSE,
                      6);

  /* TRANSLATORS: label for the Advanced expander in a service item */
  priv->advanced_expander = gtk_expander_new (_("Advanced"));
  gtk_expander_set_expanded (GTK_EXPANDER (priv->advanced_expander),
                             FALSE);
  gtk_widget_show (priv->advanced_expander);
  g_signal_connect (priv->advanced_expander,
                    "notify::expanded",
                    G_CALLBACK (_advanced_expander_notify_expanded_cb),
                    self);
  gtk_container_add (GTK_CONTAINER (align), priv->advanced_expander);

  /* TRANSLATORS: button for services that require an additional 
   * web login (clicking will open browser) */
  priv->portal_button = gtk_button_new_with_label (_("Log in"));
  g_signal_connect_after (priv->portal_button, "size-request",
                          G_CALLBACK (button_size_request_cb), self);
  g_signal_connect (priv->portal_button, "clicked",
                    G_CALLBACK (_portal_button_cb), self);
  gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      priv->portal_button,
                      FALSE, FALSE, 6);

  priv->connect_button = gtk_button_new_with_label (_("Scanning"));
  gtk_widget_show (priv->connect_button);
  g_signal_connect_after (priv->connect_button, "size-request",
                          G_CALLBACK (button_size_request_cb), self);
  g_signal_connect (priv->connect_button,
                    "clicked",
                    G_CALLBACK (_connect_button_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      priv->connect_button,
                      FALSE,
                      FALSE,
                      6);

  priv->security_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (priv->security_label),
                          0.0, 0.5);
  /* Note: security_sample should contain the longest possible security method */
  /* TRANSLATORS: This is an example of a wireless security method
     (see another translator comment), used to estimate the string length.
     It should look like the ones below network name in the UI */
  security_sample = _ ("WPA2 encrypted");
  gtk_label_set_width_chars (GTK_LABEL (priv->security_label),
                             strlen (security_sample));
  gtk_widget_show (priv->security_label);
  gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      priv->security_label,
                      FALSE,
                      FALSE,
                      6);

  /*priv->default_button = gtk_button_new ();
     gtk_widget_show (priv->default_button);
     gtk_button_set_label (GTK_BUTTON (priv->default_button),
                        _("Make default connection"));
     gtk_widget_set_sensitive (GTK_WIDGET (priv->default_button),
                            FALSE);
     gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      priv->default_button,
                      FALSE,
                      FALSE,
                      6);*/

  priv->passphrase_box = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), priv->passphrase_box,
                      TRUE, TRUE, 6);

  priv->passphrase_entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (priv->passphrase_entry), 20);
  gtk_entry_set_visibility (GTK_ENTRY (priv->passphrase_entry), TRUE);
  gtk_widget_show (priv->passphrase_entry);
  g_signal_connect (priv->passphrase_entry,
                    "activate",
                    G_CALLBACK (_passphrase_entry_activated_cb),
                    self);
  gtk_entry_set_icon_from_stock (GTK_ENTRY (priv->passphrase_entry),
                                 GTK_ENTRY_ICON_SECONDARY,
                                 GTK_STOCK_CLEAR);
  g_signal_connect (priv->passphrase_entry,
                    "icon-release",
                    G_CALLBACK (_passphrase_entry_clear_released_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (priv->passphrase_box),
                      priv->passphrase_entry,
                      FALSE, FALSE, 6);

  connect_with_pw_button = gtk_button_new_with_label (_ ("Connect"));
  gtk_widget_show (connect_with_pw_button);
  g_signal_connect_after (connect_with_pw_button, "size-request",
                          G_CALLBACK (button_size_request_cb), self);
  g_signal_connect (connect_with_pw_button,
                    "clicked",
                    G_CALLBACK (_connect_with_pw_clicked_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (priv->passphrase_box),
                      connect_with_pw_button,
                      FALSE, FALSE, 6);

  priv->show_password_check =
    gtk_check_button_new_with_label (_ ("Show password"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->show_password_check),
                                                   TRUE);
  gtk_widget_show (priv->show_password_check);
  g_signal_connect (priv->show_password_check,
                    "toggled",
                    G_CALLBACK (_show_pass_toggled_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (priv->passphrase_box),
                      priv->show_password_check,
                      FALSE, FALSE, 6);

  g_signal_connect (priv->passphrase_entry,
                    "changed",
                    G_CALLBACK (_entry_changed_cb),
                    self);

  priv->info_bar = gtk_info_bar_new ();
  gtk_widget_set_no_show_all (priv->info_bar, TRUE);
  priv->info_label = gtk_label_new ("");
  gtk_label_set_line_wrap (GTK_LABEL (priv->info_label),
                           TRUE);
  gtk_widget_show (priv->info_label);
  content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (priv->info_bar));
  gtk_container_add (GTK_CONTAINER (content_area), priv->info_label);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->info_bar,
                      FALSE, FALSE, 6);

  /* static IP UI */

  priv->advanced_box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (priv->advanced_box);
  gtk_container_add (GTK_CONTAINER (priv->expando), priv->advanced_box);

  align = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 6, 6, 20, 20);
  gtk_widget_show (align);
  gtk_box_pack_start (GTK_BOX (priv->advanced_box), align,
                      FALSE, FALSE, 0);

  table = gtk_table_new (7, 2, FALSE);
  gtk_widget_show (table);
  gtk_table_set_col_spacings (GTK_TABLE (table), 30);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_container_add (GTK_CONTAINER (align), table);

  /* TRANSLATORS: label in advanced settings (next to combobox 
   * for DHCP/Static IP) */
  add_label_to_table (GTK_TABLE (table), 0, _("Connect by:"));
  /* TRANSLATORS: label in advanced settings */
  add_label_to_table (GTK_TABLE (table), 1, _("IP address:"));
  /* TRANSLATORS: label in advanced settings */
  add_label_to_table (GTK_TABLE (table), 2, _("Subnet mask:"));
  /* TRANSLATORS: label in advanced settings */
  add_label_to_table (GTK_TABLE (table), 3, _("Router:"));
  /* TRANSLATORS: label in advanced settings */
  add_label_to_table (GTK_TABLE (table), 4, _("DNS:"));

  priv->method_combo = gtk_combo_box_new_text ();  
  /* NOTE: order/index of items in combobox is significant */
  /* TRANSLATORS: choices in the connection method combobox:
   * Will include "DHCP", "Static IP" and sometimes "Fixed IP" */
  gtk_combo_box_insert_text (GTK_COMBO_BOX (priv->method_combo),
                             METHOD_DHCP, _("DHCP"));
  gtk_combo_box_insert_text (GTK_COMBO_BOX (priv->method_combo),
                             METHOD_MANUAL, _("Static IP"));

  gtk_widget_show (priv->method_combo);
  gtk_table_attach (GTK_TABLE (table), priv->method_combo,
                    1, 2, 0, 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);
  g_signal_connect (priv->method_combo, "changed",
                    G_CALLBACK (method_combo_changed_cb), self);

  priv->address_entry = add_entry_to_table (GTK_TABLE (table), 1);
  g_signal_connect (priv->address_entry, "notify::text",
                    G_CALLBACK (static_ip_entry_notify_text), self);
  priv->netmask_entry = add_entry_to_table (GTK_TABLE (table), 2);
  g_signal_connect (priv->netmask_entry, "notify::text",
                    G_CALLBACK (static_ip_entry_notify_text), self);
  priv->gateway_entry = add_entry_to_table (GTK_TABLE (table), 3);
  g_signal_connect (priv->gateway_entry, "notify::text",
                    G_CALLBACK (static_ip_entry_notify_text), self);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled_window);
  gtk_widget_set_size_request (scrolled_window, 230, 60);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), scrolled_window,
                    1, 2, 4, 5,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);

  priv->dns_text_view = gtk_text_view_new ();
  gtk_widget_show (priv->dns_text_view);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->dns_text_view),
                               GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (priv->dns_text_view),
                                 FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->dns_text_view);
  g_signal_connect (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->dns_text_view)),
                    "changed",
                    G_CALLBACK (dns_buffer_changed_cb),
                    self);

  align = gtk_alignment_new (1.0, 0.0, 0.0, 0.0);
  gtk_widget_show (align);
  gtk_table_attach (GTK_TABLE (table), align,
                    1, 2, 6, 7,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);
  /* TRANSLATORS: label for apply button in static ip settings */
  priv->apply_button = gtk_button_new_with_label (_("Apply"));
  gtk_widget_show (priv->apply_button);
  g_signal_connect_after (priv->apply_button, "size-request",
                          G_CALLBACK (button_size_request_cb), self);
  g_signal_connect (priv->apply_button, "clicked",
                    G_CALLBACK (apply_button_clicked_cb), self);
  gtk_container_add (GTK_CONTAINER (align), priv->apply_button);
}

GtkWidget*
carrick_service_item_new (CarrickIconFactory         *icon_factory,
                          CarrickNotificationManager *notifications,
                          CarrickNetworkModel        *model,
                          GtkTreePath                *path)
{
  return g_object_new (CARRICK_TYPE_SERVICE_ITEM,
                       "icon-factory", icon_factory,
                       "notification-manager", notifications,
                       "model", model,
                       "tree-path", path,
                       NULL);
}

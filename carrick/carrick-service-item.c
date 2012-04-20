/*
 * Carrick - a connection panel for the Dawati Netbook
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
 *
 * The modem_dummy-related code should really live in another
 * file -- it's now here to get the same layout, but that could be
 * achieved bys common widget names, style classes or a common
 * inherited widget...
 */

#include "carrick-service-item.h"

#include <config.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <libnotify/notify.h>
#include "nbtk-gtk-expander.h"

#include "connman-service-bindings.h"

#include "carrick-icon-factory.h"
#include "carrick-notification-manager.h"
#include "carrick-util.h"
#include "carrick-shell.h"

#define CARRICK_ENTRY_WIDTH 160
#define CARRICK_MIN_LABEL_WIDTH 150

#define CARRICK_DRAG_TARGET "CARRICK_DRAG_TARGET"

static const GtkTargetEntry carrick_targets[] = {
  { CARRICK_DRAG_TARGET, GTK_TARGET_SAME_APP, 0 },
};

G_DEFINE_TYPE (CarrickServiceItem, carrick_service_item, GTK_TYPE_EVENT_BOX)

#define SERVICE_ITEM_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_SERVICE_ITEM, CarrickServiceItemPrivate))

enum {
  PROP_0,
  PROP_FAVORITE,
  PROP_ICON_FACTORY,
  PROP_OFONO_AGENT,
  PROP_NOTIFICATIONS,
  PROP_MODEL,
  PROP_ROW,
  PROP_ACTIVE,
  PROP_TYPE,
};

typedef enum {
  CARRICK_PASSPHRASE_WIFI,
  CARRICK_PASSPHRASE_PIN,
  CARRICK_PASSPHRASE_PUK,
  CARRICK_PASSPHRASE_NEW_PIN,
}CarrickPassphraseType;

struct _CarrickServiceItemPrivate
{
  GtkWidget *icon;
  GtkWidget *name_label;
  GtkWidget *modem_box;
  GtkWidget *connect_box;
  GtkWidget *security_label;
  GtkWidget *portal_button;
  GtkWidget *connect_button;
  GtkWidget *advanced_expander;
  GtkWidget *passphrase_box;
  GtkWidget *passphrase_entry;
  GtkWidget *show_password_check;
  GtkWidget *passphrase_button;
  GtkWidget *delete_button;
  GtkWidget *expando;

  GtkWidget *advanced_box;
  GtkWidget *method_combo;
  GtkWidget *address_entry;
  GtkWidget *gateway_entry;
  GtkWidget *netmask_entry;
  GtkWidget *ipv6_method_combo;
  GtkWidget *ipv6_address_entry;
  GtkWidget *ipv6_gateway_entry;
  GtkWidget *ipv6_prefix_length_entry;
  GtkWidget *dns_text_view;
  GtkWidget *mac_address_title_label;
  GtkWidget *mac_address_label;

  GtkWidget *proxy_none_radio;
  GtkWidget *proxy_auto_radio;
  GtkWidget *proxy_url_radio;
  GtkWidget *proxy_url_entry;
  GtkWidget *proxy_manual_radio;
  GtkWidget *proxy_manual_entry;
  GtkWidget *proxy_manual_text_view;
  GtkWidget *proxy_bypass_text_view;

  GtkWidget *apply_button;

  gboolean form_modified;

  CarrickIconFactory *icon_factory;

  gboolean is_modem_dummy;
  CarrickOfonoAgent *ofono;
  NotifyNotification *notify;
  GError *ofono_error;

  char *modem_requiring_pin;
  char *required_pin_type;

  char *entered_pin_type;
  char *entered_puk;

  char *locked_modem;
  char *locked_puk_type;

  gboolean failed;
  gboolean passphrase_hint_visible;
  CarrickPassphraseType passphrase_type;

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
  gchar *method, *address, *netmask, *gateway;
  gchar *ipv6_method, *ipv6_address, *ipv6_gateway;
  guint ipv6_prefix_length;
  gchar **nameservers;
  gboolean favorite;
  gboolean immutable;
  gboolean login_required;
  gchar *mac_address;

  gchar *proxy_method, *proxy_url;
  gchar **proxy_servers, **proxy_excludes;
  gchar *config_proxy_method, *config_proxy_url;
  gchar **config_proxy_servers, **config_proxy_excludes;

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

enum {
  IPV6_METHOD_OFF = 0,
  IPV6_METHOD_AUTO = 1,
  IPV6_METHOD_MANUAL = 2,
  IPV6_METHOD_FIXED = 3,
};

typedef enum {
  CARRICK_INET_V4,
  CARRICK_INET_V6,
  CARRICK_INET_ANY
} CarrickInet;

static void _connect_ofono_handlers (CarrickServiceItem *self);

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

    case PROP_OFONO_AGENT:
      g_value_set_object (value, priv->ofono);
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

    case PROP_TYPE:
      g_value_set_string (value, priv->type);
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
carrick_service_item_set_type (CarrickServiceItem *item, const char *type)
{
  if (g_strcmp0 (type, item->priv->type) == 0)
    return;

  if (item->priv->type)
    g_free (item->priv->type);
  item->priv->type = g_strdup (type);

  /* TODO type shouldn't change but disconnecting if it happens
   * would still be the right thing */
  _connect_ofono_handlers (item);

  g_object_notify (G_OBJECT (item), "type");
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
      char *ipv6_method, *ipv6_address, *ipv6_gateway;
      uint ipv6_prefix_length;
      char **nameservers;
      char *proxy_method, *proxy_url;
      char **proxy_servers, **proxy_excludes;
      char *state;
      char *type;

      if (priv->proxy)
        g_object_unref (priv->proxy);
      g_free (priv->name);
      g_free (priv->security);
      g_free (priv->passphrase);
      g_free (priv->method);
      g_free (priv->address);
      g_free (priv->netmask);
      g_free (priv->gateway);
      g_free (priv->ipv6_method);
      g_free (priv->ipv6_address);
      g_free (priv->ipv6_gateway);
      g_strfreev (priv->nameservers);
      g_free (priv->mac_address);

      g_free (priv->proxy_method);
      g_free (priv->proxy_url);
      g_strfreev (priv->proxy_servers);
      g_strfreev (priv->proxy_excludes);

      gtk_tree_model_get (GTK_TREE_MODEL (priv->model), &iter,
                          CARRICK_COLUMN_PROXY, &priv->proxy,
                          CARRICK_COLUMN_INDEX, &priv->index,
                          CARRICK_COLUMN_NAME, &priv->name,
                          CARRICK_COLUMN_TYPE, &type,
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
                          CARRICK_COLUMN_IPV6_METHOD, &ipv6_method,
                          CARRICK_COLUMN_IPV6_ADDRESS, &ipv6_address,
                          CARRICK_COLUMN_IPV6_PREFIX_LENGTH, &ipv6_prefix_length,
                          CARRICK_COLUMN_IPV6_GATEWAY, &ipv6_gateway,
                          CARRICK_COLUMN_CONFIGURED_IPV6_METHOD, &priv->ipv6_method,
                          CARRICK_COLUMN_CONFIGURED_IPV6_ADDRESS, &priv->ipv6_address,
                          CARRICK_COLUMN_CONFIGURED_IPV6_PREFIX_LENGTH, &priv->ipv6_prefix_length,
                          CARRICK_COLUMN_CONFIGURED_IPV6_GATEWAY, &priv->ipv6_gateway,
                          CARRICK_COLUMN_NAMESERVERS, &nameservers,
                          CARRICK_COLUMN_CONFIGURED_NAMESERVERS, &priv->nameservers,
                          CARRICK_COLUMN_FAVORITE, &priv->favorite,
                          CARRICK_COLUMN_IMMUTABLE, &priv->immutable,
                          CARRICK_COLUMN_LOGIN_REQUIRED, &priv->login_required,
                          CARRICK_COLUMN_ETHERNET_MAC_ADDRESS, &priv->mac_address,
                          CARRICK_COLUMN_PROXY_METHOD, &proxy_method,
                          CARRICK_COLUMN_PROXY_URL, &proxy_url,
                          CARRICK_COLUMN_PROXY_SERVERS, &proxy_servers,
                          CARRICK_COLUMN_PROXY_EXCLUDES, &proxy_excludes,
                          CARRICK_COLUMN_PROXY_CONFIGURED_METHOD, &priv->proxy_method,
                          CARRICK_COLUMN_PROXY_CONFIGURED_URL, &priv->proxy_url,
                          CARRICK_COLUMN_PROXY_CONFIGURED_SERVERS, &priv->proxy_servers,
                          CARRICK_COLUMN_PROXY_CONFIGURED_EXCLUDES, &priv->proxy_excludes,
                          -1);

      carrick_service_item_set_type (self, type);
      if (type)
        g_free (type);

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

      if (priv->ipv6_method)
        g_free (ipv6_method);
      else
        priv->ipv6_method = ipv6_method;
      if (priv->ipv6_address)
        g_free (ipv6_address);
      else
        priv->ipv6_address = ipv6_address;
      if (priv->ipv6_prefix_length == 0)
        priv->ipv6_prefix_length = ipv6_prefix_length;
      if (priv->ipv6_gateway)
        g_free (ipv6_gateway);
      else
        priv->ipv6_gateway = ipv6_gateway;

      if (priv->nameservers && g_strv_length (priv->nameservers) > 0)
        g_strfreev (nameservers);
      else
        {
          g_strfreev (priv->nameservers);
          priv->nameservers = nameservers;
        }

      if (priv->proxy_method)
        g_free (proxy_method);
      else
        priv->proxy_method = proxy_method;
      if (priv->proxy_url)
        g_free (proxy_url);
      else
        priv->proxy_url = proxy_url;
      if (priv->proxy_servers && g_strv_length (priv->proxy_servers) > 0)
        g_strfreev (proxy_servers);
      else
        {
          g_strfreev (priv->proxy_servers);
          priv->proxy_servers = proxy_servers;
        }
      if (priv->proxy_excludes && g_strv_length (priv->proxy_excludes) > 0)
        g_strfreev (proxy_excludes);
      else
        {
          g_strfreev (priv->proxy_excludes);
          priv->proxy_excludes = proxy_excludes;
        }

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
  gchar                     *length, *nameservers = NULL;
  GtkTextBuffer             *buf;
  GtkComboBox               *combo;

  combo = GTK_COMBO_BOX (priv->method_combo);

  /* This is a bit of a hack: we don't want to show "Fixed" as
   * option if it's not sent by connman */
  if (g_strcmp0 (priv->method, "manual") == 0)
    {
      if (gtk_combo_box_get_active (combo) == METHOD_FIXED)
        gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (combo), METHOD_FIXED);

      gtk_combo_box_set_active (combo, METHOD_MANUAL);
      gtk_widget_set_sensitive (GTK_WIDGET (combo), TRUE);
    }
  else if (g_strcmp0 (priv->method, "dhcp") == 0)
    {
      if (gtk_combo_box_get_active (combo) == METHOD_FIXED)
        gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (combo), METHOD_FIXED);

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
          gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (combo),
                                          METHOD_FIXED, _("Fixed IP"));
        }
      gtk_combo_box_set_active (combo, METHOD_FIXED);
      gtk_widget_set_sensitive (GTK_WIDGET (combo), FALSE);
    }
  else if (priv->method)
    {
      g_warning ("Unknown service ipv4 method '%s'", priv->method);
    }

  combo = GTK_COMBO_BOX (priv->ipv6_method_combo);

  /* Same hack for IPv6 */
  if (g_strcmp0 (priv->ipv6_method, "manual") == 0)
    {
      if (gtk_combo_box_get_active (combo) == IPV6_METHOD_FIXED)
        gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (combo),
                                   IPV6_METHOD_FIXED);

      gtk_combo_box_set_active (combo, IPV6_METHOD_MANUAL);
      gtk_widget_set_sensitive (GTK_WIDGET (combo), TRUE);
    }
  else if (g_strcmp0 (priv->ipv6_method, "auto") == 0)
    {
      if (gtk_combo_box_get_active (combo) == IPV6_METHOD_FIXED)
        gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (combo),
                                   IPV6_METHOD_FIXED);

      gtk_combo_box_set_active (combo, IPV6_METHOD_AUTO);
      gtk_widget_set_sensitive (GTK_WIDGET (combo), TRUE);
    }
  else if (g_strcmp0 (priv->ipv6_method, "fixed") == 0)
    {
      if (gtk_combo_box_get_active (combo) != IPV6_METHOD_FIXED)
        {
           /* TRANSLATORS: This string will be in the "Connect by:"-
            * combobox just like "DHCP" and "Static IP". Fixed means
            * that the IP configuration cannot be changed at all,
            * like in a 3G network */
          gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (combo),
                                          IPV6_METHOD_FIXED, _("Fixed IP"));
        }
      gtk_combo_box_set_active (combo, IPV6_METHOD_FIXED);
      gtk_widget_set_sensitive (GTK_WIDGET (combo), FALSE);
    }
  else if (g_strcmp0 (priv->ipv6_method, "off") == 0)
    {
      if (gtk_combo_box_get_active (combo) == IPV6_METHOD_FIXED)
        gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (combo),
                                   IPV6_METHOD_FIXED);

      gtk_combo_box_set_active (combo, IPV6_METHOD_OFF);
      gtk_widget_set_sensitive (GTK_WIDGET (combo), TRUE);
    }
  else if (priv->ipv6_method)
    {
      g_warning ("Unknown service ipv6 method '%s'", priv->ipv6_method);
    }

  gtk_entry_set_text (GTK_ENTRY (priv->address_entry),
                      priv->address ? priv->address : "");

  gtk_entry_set_text (GTK_ENTRY (priv->netmask_entry),
                      priv->netmask ? priv->netmask : "");

  gtk_entry_set_text (GTK_ENTRY (priv->gateway_entry),
                      priv->gateway ? priv->gateway : "");

  gtk_entry_set_text (GTK_ENTRY (priv->ipv6_address_entry),
                      priv->ipv6_address ? priv->ipv6_address : "");
  gtk_entry_set_text (GTK_ENTRY (priv->ipv6_gateway_entry),
                      priv->ipv6_gateway ? priv->ipv6_gateway : "");

  if (priv->ipv6_prefix_length == 0)
    length = g_strdup ("");
  else
    length = g_strdup_printf ("%u", priv->ipv6_prefix_length);
  gtk_entry_set_text (GTK_ENTRY (priv->ipv6_prefix_length_entry),
                      length);
  g_free (length);

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

   if (priv->mac_address && strlen (priv->mac_address) > 0)
     {
       gtk_label_set_text (GTK_LABEL (priv->mac_address_label),
                           priv->mac_address);
       gtk_widget_show (priv->mac_address_label);
       gtk_widget_show (priv->mac_address_title_label);
     }
   else
     {
       gtk_widget_hide (priv->mac_address_label);
       gtk_widget_hide (priv->mac_address_title_label);
     }

  if (g_strcmp0 (priv->proxy_method, "direct") == 0)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->proxy_none_radio),
                                    TRUE);
    }
  else if (g_strcmp0 (priv->proxy_method, "manual") == 0)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->proxy_manual_radio),
                                    TRUE);
    }
  else if (g_strcmp0 (priv->proxy_method, "auto") == 0)
    {
      if (priv->proxy_url && strlen (priv->proxy_url) > 0)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->proxy_url_radio),
                                      TRUE);
      else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->proxy_auto_radio),
                                      TRUE);
    }
  else if (priv->proxy_method)
    {
      g_warning ("Unknown proxy method '%s'", priv->proxy_method);
    }

  gtk_entry_set_text (GTK_ENTRY (priv->proxy_url_entry),
                      priv->proxy_url ? priv->proxy_url : "");

  if (priv->proxy_servers)
    {
      char *servers;
      servers = g_strjoinv (", ", priv->proxy_servers);
      gtk_entry_set_text (GTK_ENTRY (priv->proxy_manual_entry), servers);
      g_free (servers);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (priv->proxy_manual_entry), "");
    }

  buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->proxy_bypass_text_view));
  if (priv->proxy_excludes)
    {
      char *proxy_excludes;
      proxy_excludes = g_strjoinv ("\n", priv->proxy_excludes);
      gtk_text_buffer_set_text (buf, proxy_excludes, -1);
      g_free (proxy_excludes);
    }
  else
    {
      gtk_text_buffer_set_text (buf, "", -1);
    }

  /* need to initialize this after setting the widgets, as their signal
   * handlers set form_modified to TRUE */
  _set_form_modified (self, FALSE);
}

static char*
carrick_service_item_build_pin_required_message (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;
  char                      *msg = NULL;

  if (priv->locked_puk_type)
    {
      /* TRANSLATORS: error when a puk code is locked. Placeholder is usually "puk",
       * see pin entry translations for full list of possible values.  */
      msg = g_strdup_printf (_("The %s code is locked, contact your service provider."),
                             carrick_ofono_prettify_pin (priv->locked_puk_type));
    }
  else if (priv->required_pin_type)
    {
      int retries;
      char *err_name = NULL;

      if (priv->ofono_error)
        err_name = g_dbus_error_get_remote_error (priv->ofono_error);
      retries = carrick_ofono_agent_get_retries (priv->ofono,
                                                 priv->modem_requiring_pin,
                                                 priv->required_pin_type);

      if (carrick_ofono_is_pin (priv->required_pin_type))
        {
          if (retries > 0 && retries < 3)
            {
              /* TRANSLATORS: info message when pin entry is required and
               * there are less than three retries,
               * Placeholder is pin type, usually "PIN" */
              if (g_strcmp0 (err_name, "org.ofono.Error.InvalidFormat") == 0)
                msg = g_strdup_printf (ngettext ("Sorry, that does not look like a valid %1$s code. A %2$s code is still required to unlock the SIM card. You can try once more before the code is locked.",
                                                 "Sorry, that does not look like a valid %1$s code. A %2$s code is still required to unlock the SIM card. You can try two more times before the code is locked.",
                                                 retries),
                                     carrick_ofono_prettify_pin (priv->entered_pin_type),
                                     carrick_ofono_prettify_pin (priv->required_pin_type));
              if (g_strcmp0 (err_name, "org.ofono.Error.Failed") == 0)
                msg = g_strdup_printf (ngettext ("Sorry, the %1$s code is incorrect. A %2$s code is still required to unlock the SIM card. You can try once more before the code is locked.",
                                                 "Sorry, the %1$s code is incorrect. A %2$s code is still required to unlock the SIM card. You can try two more times before the code is locked.",
                                                 retries),
                                     carrick_ofono_prettify_pin (priv->entered_pin_type),
                                     carrick_ofono_prettify_pin (priv->required_pin_type));
              else
                msg = g_strdup_printf (ngettext ("A %s code is required to unlock the SIM card. You can try once more before the code is locked.",
                                                 "A %s code is required to unlock the SIM card. You can try two more times before the code is locked.",
                                                 retries),
                                       carrick_ofono_prettify_pin (priv->required_pin_type));
            }
          else
            {
              /* only invalidformat-error makes sense when retries is still full */
              /* TRANSLATORS: info message when pin entry is required,
               * Placeholders are pin types, usually "PIN" */
              if (g_strcmp0 (err_name, "org.ofono.Error.InvalidFormat") == 0)
                msg = g_strdup_printf (_("Sorry, that does not look like a valid %1$s code. A %2$s code is still required to unlock the SIM card."),
                                       carrick_ofono_prettify_pin (priv->entered_pin_type),
                                       carrick_ofono_prettify_pin (priv->required_pin_type));
              else
                msg = g_strdup_printf (_("A %s code is required to unlock the SIM card."),
                                       carrick_ofono_prettify_pin (priv->required_pin_type));
            }
        }
      else
        {
          if (priv->passphrase_type == CARRICK_PASSPHRASE_NEW_PIN)
            {
              /* TRANSLATORS: info message when user has entered a PUK code and
               * now needs to set a new PIN code */
              msg = g_strdup_printf (_("Now enter the new %s code"),
                                     carrick_ofono_prettify_pin (carrick_ofono_pin_for_puk (priv->required_pin_type)));
            }
          else if (retries > 0 && retries < 10)
            {

              /* TRANSLATORS: info message when pin reset is required and
               * there are less than 10 retries
               * Placeholder 1 & 2 are puk type, usually "PUK"
               * Placeholder 3 is pin type, usually "PIN" */
              if (g_strcmp0 (err_name, "org.ofono.Error.InvalidFormat") == 0)
                msg = g_strdup_printf (ngettext ("Sorry, that does not look like a valid %1$s code. A %2$s code is still required to reset the %3$s code. You can try once more before the code is permanently locked.",
                                                 "Sorry, that does not look like a valid %1$s code. A %2$s code is still required to reset the %3$s code. You can try %4$d more times before the code is permanently locked.",
                                                 retries),
                                       carrick_ofono_prettify_pin (priv->entered_pin_type),
                                       carrick_ofono_prettify_pin (priv->required_pin_type),
                                       carrick_ofono_prettify_pin (carrick_ofono_pin_for_puk (priv->required_pin_type)),
                                       retries);
              else if (g_strcmp0 (err_name, "org.ofono.Error.Failed") == 0)
                msg = g_strdup_printf (ngettext ("Sorry, the %1$s code is incorrect. A %2$s code is still required to reset the %3$s code. You can try once more before the code is permanently locked.",
                                                 "Sorry, the %1$s code is incorrect. A %2$s code is still required to reset the %3$s code. You can try %4$d more times before the code is permanently locked.",
                                                 retries),
                                       carrick_ofono_prettify_pin (priv->entered_pin_type),
                                       carrick_ofono_prettify_pin (priv->required_pin_type),
                                       carrick_ofono_prettify_pin (carrick_ofono_pin_for_puk (priv->required_pin_type)),
                                       retries);
              else
                msg = g_strdup_printf (ngettext ("A %1$s code is required to reset the %2$s code. You can try once more before the code is permanently locked.",
                                                 "A %1$s code is required to reset the %2$s code. You can try %3$d more times before the code is permanently locked.",
                                                 retries),
                                       carrick_ofono_prettify_pin (priv->required_pin_type),
                                       carrick_ofono_prettify_pin (carrick_ofono_pin_for_puk (priv->required_pin_type)),
                                       retries);
            }
          else
            {
              /* TRANSLATORS: info message when pin reset is required,
               * Placeholder 1 & 3 are pin type, usually "PIN"
               * Placeholder 2 is puk type, usually "PUK" */
              if (g_strcmp0 (err_name, "org.ofono.Error.InvalidFormat") == 0)
                msg = g_strdup_printf (_("Sorry, that does not look like a valid %1$s code. A %2$s code is still required to reset the %3$s code."),
                                       carrick_ofono_prettify_pin (priv->entered_pin_type),
                                       carrick_ofono_prettify_pin (priv->required_pin_type),
                                       carrick_ofono_prettify_pin (carrick_ofono_pin_for_puk (priv->required_pin_type)));
              else if (g_strcmp0 (err_name, "org.ofono.Error.Failed") == 0)
                msg = g_strdup_printf (_("Sorry, that %1$s code is incorrect. A %2$s code is required to reset the %3$s code."),
                                       carrick_ofono_prettify_pin (priv->entered_pin_type),
                                       carrick_ofono_prettify_pin (priv->required_pin_type),
                                       carrick_ofono_prettify_pin (carrick_ofono_pin_for_puk (priv->required_pin_type)));
              else
                msg = g_strdup_printf (_("A %1$s code is required to reset the %2$s code."),
                                       carrick_ofono_prettify_pin (priv->required_pin_type),
                                       carrick_ofono_prettify_pin (carrick_ofono_pin_for_puk (priv->required_pin_type)));
            }
        }
      g_free (err_name);
    }
  else
    {
      msg = g_strdup ("");
    }

  return msg;
}

static void
_request_passphrase (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;
  const char *pretty_pin;
  char *hint = NULL;
  char *btn_text = NULL;
  char *check_text = NULL;

  switch (priv->passphrase_type)
  {
  case CARRICK_PASSPHRASE_WIFI:
    /* TRANSLATORS: text is used as a hint in the passphrase entry and
     * should be 20 characters or less to be entirely visible */
    hint = g_strdup (_("Type password here"));
    /* TRANSLATORS: Button label when connecting with passphrase */
    btn_text = g_strdup (_("Connect"));
    /* TRANSLATORS: A check button label when connecting wifi */
    check_text = g_strdup (_("Show password"));
    break;
  case CARRICK_PASSPHRASE_PIN:
  case CARRICK_PASSPHRASE_PUK:
    pretty_pin = carrick_ofono_prettify_pin (item->priv->required_pin_type);
    /* TRANSLATORS: text is used as a hint in the PIN entry and
     * should be 20 characters or less to be entirely visible
     * The placeholder is usually "PIN" or "PUK" but it
     * can be any of these:
     * "PIN", "PHONE", "FIRSTPHONE", "PIN2", "NETWORK",
     * "NETSUB", "SERVICE", "CORP"
     * "PUK", "FIRSTPHONEPUK", "PUK2", "NETWORKPUK",
     * "NETSUBPUK", "SERVICEPUK", "CORPPUK",
     */
    hint = g_strdup_printf (_("Type %s here"), pretty_pin);
    /* TRANSLATORS: Button label when entering a PIN code. Placeholder as above */
    btn_text = g_strdup_printf (_("Enter %s"), pretty_pin);
    /* TRANSLATORS: A check button label when connecting a cellular modem.
     * Placeholder as above */
    check_text = g_strdup_printf (_("Show %s"), pretty_pin);
    break;
  case CARRICK_PASSPHRASE_NEW_PIN:
    pretty_pin = carrick_ofono_prettify_pin (carrick_ofono_pin_for_puk (item->priv->required_pin_type));
    /* TRANSLATORS: text is used as a hint in the PIN entry when setting
     * a new pin. Same rules as above for normal PIN entry. */
    hint = g_strdup_printf (_("Type new %s here"), pretty_pin);
    /* TRANSLATORS: Button label when entering new pin code.
     * Placeholder as above */
    btn_text = g_strdup_printf (_("Enter new %s"), pretty_pin);
    /* TRANSLATORS: A check button label when entering a new pin code.
     * Placeholder as above */
    check_text = g_strdup_printf (_("Show %s"), pretty_pin);
    break;
  }

  gtk_button_set_label (GTK_BUTTON (item->priv->passphrase_button),
                        btn_text);
  gtk_button_set_label (GTK_BUTTON (item->priv->show_password_check),
                        check_text);

  if (!priv->passphrase || (strlen (priv->passphrase) == 0))
    {
      priv->passphrase_hint_visible = TRUE;
      gtk_entry_set_text (GTK_ENTRY (priv->passphrase_entry), hint);
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
  gtk_widget_hide (priv->modem_box);
  gtk_widget_show (priv->passphrase_box);

  g_free (hint);
  g_free (btn_text);
  g_free (check_text);
}

static void
carrick_service_item_update_cellular_info (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;
  char                      *msg;

  if (priv->locked_puk_type)
    gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                   GTK_MESSAGE_ERROR);
  else
    gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                   GTK_MESSAGE_INFO);

  msg = carrick_service_item_build_pin_required_message (item);
  gtk_label_set_text (GTK_LABEL (priv->info_label), msg);
  g_free (msg);

  /* passphrase_type is set already */
  if (priv->required_pin_type)
    _request_passphrase (item);
  else
    gtk_widget_hide (item->priv->passphrase_box);
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
  gtk_widget_set_visible (priv->modem_box, priv->is_modem_dummy);

  if (g_strcmp0 ("ethernet", priv->type) == 0)
    {
      g_free (name);
      /* TRANSLATORS: service type for building the service title */
      name = g_strdup (_ ("Wired"));
    }
  else if (priv->is_modem_dummy)
    {
      g_free (name);
      /* TRANSLATORS: service type for building the service title */
      name = g_strdup (_("3G"));
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
      /* TRANSLATORS: service state string, will be used as a title
       * like this: "MyWifiAP - Connected" */
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
      /* TRANSLATORS: button label when service is online */
      button = g_strdup (_ ("Disconnect"));
      /* TRANSLATORS: service state string, see example above */
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Online"));
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else if (g_strcmp0 (priv->state, "configuration") == 0)
    {
      /* TRANSLATORS: button label when service is configuring*/
      button = g_strdup_printf (_ ("Cancel"));
      /* TRANSLATORS: service state string, see example above */
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Configuring"));
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else if (g_strcmp0 (priv->state, "association") == 0)
    {
      /* TRANSLATORS: button label when service is associating */
      button = g_strdup_printf (_("Cancel"));
      /* TRANSLATORS: service state string, see example above */
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
      /* TRANSLATORS: button label when service is not connected */
      button = g_strdup (_ ("Connect"));
      label = g_strdup (name);

      gtk_label_set_text (GTK_LABEL (priv->info_label), "");

      /* the dummy modem service is always in idle */
      /* TODO can't trust this to be tru as we want some
       * updates to happen for cell services -- which might be in
       * other states as well*/
      carrick_service_item_update_cellular_info (self);
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

      /* TRANSLATORS: button label when service failed to connect */
      button = g_strdup (_ ("Connect"));
      /* TRANSLATORS: service state string, see example above */
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
      /* TRANSLATORS: button label when service is dicsonnecting
       * and is insensitive */
      button = g_strdup_printf (_ ("Disconnecting"));
      /* TRANSLATORS: service state string, see example above */
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
  GtkWidget                 *dialog, *content;
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
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  carrick_shell_close_dialog_on_hide (GTK_DIALOG (dialog));

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

  gtk_box_set_spacing (GTK_BOX (content), 12);
  gtk_box_pack_start (GTK_BOX (content),
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
          priv->passphrase_type = CARRICK_PASSPHRASE_WIFI;
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
      carrick_notification_manager_queue_event (priv->note,
                                                priv->type,
                                                "ready",
                                                priv->name);
      _connect (item);
    }
}

static void
_modem_button_cb (GtkButton          *modem_button,
                  CarrickServiceItem *item)
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
    }
  else
   {
      g_warning ("Unable to spawn 3g connection wizard");
      g_error_free(error);
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
          gtk_widget_set_visible (priv->connect_box, !priv->is_modem_dummy);

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
_notify_action_cb (NotifyNotification *notification,
                   char *action,
                   CarrickServiceItem *item)
{
  /* user clicked "Enter PIN" in the notification */
  carrick_shell_show ();
  gtk_widget_grab_focus (item->priv->passphrase_entry);
}

static void
carrick_service_desktop_notify_request_pin (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;
  char *title, *message, *action;
  const char *icon = NULL;

  /* if there's no pin request we don't notify */
  if (!item->priv->required_pin_type || priv->locked_puk_type) {
    if (priv->notify)
      notify_notification_close (priv->notify, NULL);
    return;
  }

  /* if the panel is open we don't notify */
  if (carrick_shell_is_visible ())
    return;

  if (!priv->notify) {
#ifdef HAVE_NOTIFY_0_7
    priv->notify = notify_notification_new ("", NULL, NULL);
#else
    priv->notify = notify_notification_new ("", NULL, NULL, NULL);
#endif
  }

  /*TRANSLATORS: desktop notification title when a pin/puk code is needed.
   * Placeholder is a pin/puk type string */
  title = g_strdup_printf (_("%s code is needed"),
                           carrick_ofono_prettify_pin (priv->required_pin_type));
  message = carrick_service_item_build_pin_required_message (item);

  notify_notification_update (priv->notify,
                              title,
                              message,
                              icon);

  notify_notification_clear_actions (priv->notify);
  /*TRANSLATORS: action button in desktop notification when a pin/puk
   * code is needed. Placeholder is a pin/puk type string */
  action = g_strdup_printf (_("Enter %s"),
                            carrick_ofono_prettify_pin (priv->required_pin_type));
  notify_notification_add_action (priv->notify,
                                  "enter-pin", action,
                                  (NotifyActionCallback)_notify_action_cb,
                                  item, NULL);

  notify_notification_show (priv->notify, NULL);
  g_free (title);
  g_free (message);
  g_free (action);
}

static gboolean
_wait_for_ofono_to_settle (CarrickServiceItem *item)
{
  carrick_service_item_update (item);
  return FALSE;
}

static void
_ofono_agent_enter_pin_cb (CarrickOfonoAgent *agent,
                         const char *modem_path,
                         GAsyncResult *res,
                         CarrickServiceItem *item)
{
  GError *error = NULL;

  g_clear_error (&item->priv->ofono_error);

  if (!carrick_ofono_agent_finish (agent, modem_path, res, &error)) {
    if (!error)
      g_warning ("EnterPin() failed without message");

    /* save error so it can be used in _update_cellular_info() */
    item->priv->ofono_error = error;
  }

  /* it's possible that a pin is still required, but nothing in ofono
   * API changes so we must ensure the message is shown... but we want
   * to avoid e.g. setting an error message and then immediately
   * changing the message contents: so we wait just a short while
   * before we ensure the error is shown */
  g_timeout_add (200, (GSourceFunc)_wait_for_ofono_to_settle, item);
}

static void
_ofono_agent_reset_pin_cb (CarrickOfonoAgent *agent,
                         const char *modem_path,
                         GAsyncResult *res,
                         CarrickServiceItem *item)
{
  GError *error = NULL;

  if (!carrick_ofono_agent_finish (agent, modem_path, res, &error)) {
    if (!error)
      g_warning ("ResetPin() failed without message");
    else

    /* save error so it can be used in _update_cellular_info() */
    item->priv->ofono_error = error;
  }

  /* it's possible that a puk is still required, but nothing in ofono
   * API changes so we must ensure the message is shown... but we want
   * to avoid e.g. setting an error message and then immediately
   * changing the message contents: so we wait just a short while
   * before we ensure the error is shown */
  g_timeout_add (200, (GSourceFunc)_wait_for_ofono_to_settle, item);
}

static void
carrick_service_item_enter_pin (CarrickServiceItem *item)
{
  const char *pin;
  pin = gtk_entry_get_text (GTK_ENTRY (item->priv->passphrase_entry));

  /* we let ofono do the validation */

  if (item->priv->entered_pin_type)
    g_free (item->priv->entered_pin_type);
  item->priv->entered_pin_type = g_strdup (item->priv->required_pin_type);

  carrick_ofono_agent_enter_pin (item->priv->ofono,
                                 item->priv->modem_requiring_pin,
                                 item->priv->required_pin_type,
                                 pin,
                                 (CarrickOfonoAgentCallback)_ofono_agent_enter_pin_cb,
                                 item);
  gtk_widget_hide (item->priv->passphrase_box);
}

static void
carrick_service_item_reset_pin (CarrickServiceItem *item)
{
  const char *new_pin;
  new_pin = gtk_entry_get_text (GTK_ENTRY (item->priv->passphrase_entry));

  /* we let ofono do the validation */

  if (item->priv->entered_pin_type)
    g_free (item->priv->entered_pin_type);
  item->priv->entered_pin_type = g_strdup (item->priv->required_pin_type);
  carrick_ofono_agent_reset_pin (item->priv->ofono,
                                 item->priv->modem_requiring_pin,
                                 item->priv->required_pin_type,
                                 item->priv->entered_puk,
                                 new_pin,
                                 (CarrickOfonoAgentCallback)_ofono_agent_reset_pin_cb,
                                 item);
  gtk_widget_hide (item->priv->passphrase_box);

  g_free (item->priv->entered_puk);
  item->priv->entered_puk = NULL;
}

static void
_passphrase_button_or_entry_cb (GtkWidget *button_or_entry,
                                CarrickServiceItem *item)
{
  const char *puk;

  /* make sure we don't show stale error messages for cell service / modem dummy */
  g_clear_error (&item->priv->ofono_error);

  carrick_service_item_set_active (item, TRUE);
  switch (item->priv->passphrase_type)
    {
    case CARRICK_PASSPHRASE_WIFI:
      _connect_with_password (item);
      break;
    case CARRICK_PASSPHRASE_PIN:
      carrick_service_item_enter_pin (item);
      break;
    case CARRICK_PASSPHRASE_PUK:
      puk = gtk_entry_get_text (GTK_ENTRY (item->priv->passphrase_entry));
      item->priv->entered_puk = g_strdup (puk);
      item->priv->passphrase_type = CARRICK_PASSPHRASE_NEW_PIN;
      carrick_service_item_update (item);
      break;
    case CARRICK_PASSPHRASE_NEW_PIN:
      carrick_service_item_reset_pin (item);
      break;
    }
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
      gtk_widget_set_visible (priv->connect_box, !priv->is_modem_dummy);
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
carrick_service_item_set_locked_puk_type (CarrickServiceItem *item,
                                          const char* obj_path,
                                          const char* puk_type)
{
  if (item->priv->locked_modem)
    g_free (item->priv->locked_modem);
  if (item->priv->locked_puk_type)
    g_free (item->priv->locked_puk_type);

  item->priv->locked_modem = g_strdup (obj_path);
  item->priv->locked_puk_type = g_strdup (puk_type);

  if (!obj_path)
    return;

  carrick_service_item_update (item);
}

static void
carrick_service_item_set_required_pin_type (CarrickServiceItem *item,
                                            const char* obj_path,
                                            const char* pin_type)
{
  if (g_strcmp0 (item->priv->modem_requiring_pin, obj_path) == 0 &&
      g_strcmp0 (item->priv->required_pin_type, pin_type) == 0)
    return;

  if (item->priv->modem_requiring_pin)
    g_free (item->priv->modem_requiring_pin);
  if (item->priv->required_pin_type)
    g_free (item->priv->required_pin_type);

  item->priv->modem_requiring_pin = g_strdup (obj_path);
  item->priv->required_pin_type = g_strdup (pin_type);

  /* need to set passphrase_type here and not later so we do not
   * unnecessarily overwrite the CARRICK_PASSPHRASE_NEW_PIN that
   * may be set */
  if (carrick_ofono_is_pin (pin_type))
    item->priv->passphrase_type = CARRICK_PASSPHRASE_PIN;
  else if (carrick_ofono_is_puk (pin_type))
    item->priv->passphrase_type = CARRICK_PASSPHRASE_PUK;

  carrick_service_item_update (item);

  carrick_service_desktop_notify_request_pin (item);
}


static void
_ofono_notify_required_pins_cb (CarrickOfonoAgent *ofono,
                                GParamSpec *arg1,
                                CarrickServiceItem *self)
{
  GHashTable *required_pins;
  GHashTableIter iter;
  gpointer key, val;

  if (!self->priv->is_modem_dummy)
    return;

  g_object_get (ofono, "required-pins", &required_pins, NULL);
  g_hash_table_iter_init (&iter, required_pins);

  /* note: only deals with the first required pin or null-case */
  key = val = NULL;
  g_hash_table_iter_next (&iter, &key, &val);
  carrick_service_item_set_required_pin_type (self, (const char*)key, (const char*)val);
}

static void
_ofono_notify_locked_puks_cb (CarrickOfonoAgent *ofono,
                              GParamSpec *arg1,
                              CarrickServiceItem *self)
{
  GHashTable *locked_puks;
  GHashTableIter iter;
  gpointer key, val;

  if (!self->priv->is_modem_dummy)
    return;

  g_object_get (ofono, "locked-puks", &locked_puks, NULL);
  g_hash_table_iter_init (&iter, locked_puks);

  /* note: only deals with the first locked puk or the null-case */
  key = val = NULL;
  g_hash_table_iter_next (&iter, &key, &val);
  carrick_service_item_set_locked_puk_type (self, (const char*)key, (const char*)val);
}

static void
_ofono_retries_changed_cb (CarrickOfonoAgent *ofono,
                           CarrickServiceItem *item)
{
  /* need to update ui since we may have a error message up showing the
     retry count */
  carrick_service_item_update (item);
}

static void
_connect_ofono_handlers (CarrickServiceItem *self)
{
  if (self->priv->ofono && g_strcmp0 (self->priv->type, "cellular") == 0) {
    g_signal_connect (self->priv->ofono, "notify::required-pins",
                      G_CALLBACK (_ofono_notify_required_pins_cb), self);
    _ofono_notify_required_pins_cb (self->priv->ofono, NULL, self);
    g_signal_connect (self->priv->ofono, "notify::locked-puks",
                      G_CALLBACK (_ofono_notify_locked_puks_cb), self);
    g_signal_connect (self->priv->ofono, "retries-changed",
                      G_CALLBACK (_ofono_retries_changed_cb), self);
    _ofono_notify_locked_puks_cb (self->priv->ofono, NULL, self);
  }

}

static void
_set_model (CarrickServiceItem  *self,
            CarrickNetworkModel *model)
{
  CarrickServiceItemPrivate *priv = self->priv;

  priv->model = model;

  if (priv->row)
     _populate_variables (self);
}

static void
_set_row (CarrickServiceItem *self,
          GtkTreePath        *path)
{
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
    _populate_variables (self);
}

void
carrick_service_item_update (CarrickServiceItem *self)
{
  if (self->priv->model)
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

    case PROP_OFONO_AGENT:
      priv->ofono = CARRICK_OFONO_AGENT (g_value_dup_object (value));
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
      g_object_unref (priv->hand_cursor);
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
  g_free (priv->ipv6_method);
  priv->ipv6_method = NULL;
  g_free (priv->ipv6_address);
  priv->ipv6_address = NULL;
  g_free (priv->ipv6_gateway);
  priv->ipv6_gateway = NULL;
  g_strfreev (priv->nameservers);
  priv->nameservers = NULL;
  g_free (priv->mac_address);
  priv->mac_address = NULL;

  g_free (priv->proxy_method);
  priv->proxy_method = NULL;
  g_free (priv->proxy_url);
  priv->proxy_url = NULL;
  g_strfreev (priv->proxy_servers);
  priv->proxy_servers = NULL;
  g_strfreev (priv->proxy_excludes);
  priv->proxy_excludes = NULL;

  if (priv->ofono) {
    g_signal_handlers_disconnect_by_func (priv->ofono,
                                          G_CALLBACK (_ofono_notify_required_pins_cb),
                                          object);
    g_signal_handlers_disconnect_by_func (priv->ofono,
                                          G_CALLBACK (_ofono_notify_locked_puks_cb),
                                          object);
    g_signal_handlers_disconnect_by_func (priv->ofono,
                                          G_CALLBACK (_ofono_retries_changed_cb),
                                          object);
    g_object_unref (priv->ofono);
    priv->ofono = NULL;
  }

  G_OBJECT_CLASS (carrick_service_item_parent_class)->dispose (object);
}

static gboolean
carrick_service_item_enter_notify_event (GtkWidget        *widget,
                                         GdkEventCrossing *event)
{
  CarrickServiceItem        *item = CARRICK_SERVICE_ITEM (widget);
  CarrickServiceItemPrivate *priv = item->priv;

  if (priv->favorite)
    gdk_window_set_cursor (gtk_widget_get_window (widget), priv->hand_cursor);
  else
    gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);

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
      gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);
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
      /* revert to last received data */
      gtk_entry_set_text (GTK_ENTRY (priv->address_entry),
                          priv->address ? priv->address : "");

      gtk_entry_set_text (GTK_ENTRY (priv->netmask_entry),
                          priv->netmask ? priv->netmask : "");

      gtk_entry_set_text (GTK_ENTRY (priv->gateway_entry),
                          priv->gateway ? priv->gateway : "");
    }

  _set_form_modified (CARRICK_SERVICE_ITEM (user_data), TRUE);
}

static void
ipv6_method_combo_changed_cb (GtkComboBox *combobox,
                              gpointer     user_data)
{
  CarrickServiceItemPrivate *priv;
  gboolean manual;

  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));
  priv = CARRICK_SERVICE_ITEM (user_data)->priv;

  manual = (gtk_combo_box_get_active (combobox) == IPV6_METHOD_MANUAL);

  gtk_widget_set_sensitive (priv->ipv6_address_entry, manual);
  gtk_widget_set_sensitive (priv->ipv6_prefix_length_entry, manual);
  gtk_widget_set_sensitive (priv->ipv6_gateway_entry, manual);

  if (!manual)
    {
      char *length;

      /* revert to last received data */
      gtk_entry_set_text (GTK_ENTRY (priv->ipv6_address_entry),
                          priv->ipv6_address ? priv->ipv6_address : "");
      gtk_entry_set_text (GTK_ENTRY (priv->ipv6_gateway_entry),
                          priv->ipv6_gateway ? priv->ipv6_gateway : "");

      if (priv->ipv6_prefix_length == 0)
        length = g_strdup ("");
      else
        length = g_strdup_printf ("%u", priv->ipv6_prefix_length);
      gtk_entry_set_text (GTK_ENTRY (priv->ipv6_prefix_length_entry),
                          length);
      g_free (length);
    }

  /* TODO hide entries if IPV6_METHOD_OFF */

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
set_configuration_cb (DBusGProxy *proxy,
                      GError     *error,
                      gpointer    data)

{
  if (error)
    {
      /* TODO: errors we should show in UI? */
      g_warning ("Error setting %s: %s",
                 (const char*)data, error->message);
      g_error_free (error);
      return;
    }
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

static gboolean
validate_static_ipv6_entries (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv;
  const char *address;

  priv = item->priv;
  address = gtk_entry_get_text (GTK_ENTRY (priv->ipv6_address_entry));
  if (!validate_address (address, CARRICK_INET_V6, FALSE))
    {
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          _("Sorry, it looks like the IPv6 address is not valid"));
      gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                     GTK_MESSAGE_WARNING);
      gtk_widget_show (priv->info_bar);
      gtk_widget_grab_focus (priv->ipv6_address_entry);
      return FALSE;
    }

  address = gtk_entry_get_text (GTK_ENTRY (priv->ipv6_gateway_entry));
  if (!validate_address (address, CARRICK_INET_V6, TRUE))
    {
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          _("Sorry, it looks like the IPv6 gateway address is not valid"));
      gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                     GTK_MESSAGE_WARNING);
      gtk_widget_show (priv->info_bar);
      gtk_widget_grab_focus (priv->ipv6_gateway_entry);
      return FALSE;
    }

  return TRUE;
}

static char**
get_ip_addr_strv (GtkTextView *dns_text_view)
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

  dnsv = get_ip_addr_strv (GTK_TEXT_VIEW (priv->dns_text_view));

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
entry_notify_text (GtkEntry   *entry,
                   GParamSpec *pspec,
                   gpointer    user_data)
{
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));

  _set_form_modified (CARRICK_SERVICE_ITEM (user_data), TRUE);
}

static void
text_view_buffer_changed_cb (GtkTextBuffer *textbuffer,
                             gpointer       user_data)
{
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));

  _set_form_modified (CARRICK_SERVICE_ITEM (user_data), TRUE);
}

static void
proxy_radio_toggled_cb (GtkWidget *radio,
                        gpointer user_data)
{
  CarrickServiceItem *item;

  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));
  item = CARRICK_SERVICE_ITEM (user_data);

  if (radio == item->priv->proxy_url_radio) {
    gtk_widget_show (item->priv->proxy_url_entry);
    gtk_widget_hide (item->priv->proxy_manual_entry);
  } else if (radio == item->priv->proxy_manual_radio) {
    gtk_widget_hide (item->priv->proxy_url_entry);
    gtk_widget_show (item->priv->proxy_manual_entry);
  } else {
    gtk_widget_hide (item->priv->proxy_url_entry);
    gtk_widget_hide (item->priv->proxy_manual_entry);
  }

  _set_form_modified (CARRICK_SERVICE_ITEM (user_data), TRUE);
}

static void
carrick_service_item_save_proxy_settings (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv;
  GValue *value, *proxy_method_value, *proxy_url_value, *proxies_value, *excludes_value;
  GHashTable *dict;
  char **proxiesv, **excludesv;
  const char *proxy_url;

  priv = item->priv;
  dict = g_hash_table_new (g_str_hash, g_str_equal);

  proxy_method_value = g_new0 (GValue, 1);
  g_value_init (proxy_method_value, G_TYPE_STRING);

  proxy_url_value = g_new0 (GValue, 1);
  g_value_init (proxy_url_value, G_TYPE_STRING);
  proxy_url = gtk_entry_get_text (GTK_ENTRY (priv->proxy_url_entry));
  g_value_set_string (proxy_url_value, proxy_url);

  excludes_value = g_new0 (GValue, 1);
  g_value_init (excludes_value, G_TYPE_STRV);
  excludesv = get_ip_addr_strv (GTK_TEXT_VIEW (priv->proxy_bypass_text_view));
  g_value_set_boxed (excludes_value, excludesv);

  proxies_value = g_new0 (GValue, 1);
  g_value_init (proxies_value, G_TYPE_STRV);
  proxiesv = g_strsplit_set (gtk_entry_get_text (GTK_ENTRY (priv->proxy_manual_entry)),
                             " ,;\n", -1);
  g_value_set_boxed (proxies_value, proxiesv);


  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->proxy_manual_radio)))
    {
      g_value_set_static_string (proxy_method_value, "manual");
      g_hash_table_insert (dict, "Servers", proxies_value);
      g_hash_table_insert (dict, "Excludes", excludes_value);
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->proxy_url_radio)))
    {
      g_value_set_static_string (proxy_method_value, "auto");
      g_hash_table_insert (dict, "URL", proxy_url_value);
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->proxy_auto_radio)))
    {
      g_value_set_static_string (proxy_method_value, "auto");
    }
  else
    {
      g_value_set_static_string (proxy_method_value, "direct");
    }

  g_hash_table_insert (dict, "Method", proxy_method_value);


  value = g_new0 (GValue, 1);
  g_value_init (value, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE));
  g_value_set_boxed (value, dict);

  net_connman_Service_set_property_async (priv->proxy,
                                          "Proxy.Configuration",
                                          value,
                                          set_configuration_cb,
                                          "Proxy.Configuration");


  g_free (proxy_url_value);
  g_free (proxy_method_value);
  g_free (excludes_value);
  g_strfreev (excludesv);
  g_free (proxies_value);
  g_strfreev (proxiesv);
  g_free (value);
  g_hash_table_destroy (dict);
}


static void
carrick_service_item_save_ip_settings (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv;
  GValue *value;
  GHashTable *dict;
  int method;

  priv = item->priv;
  method = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->method_combo));

  if (method != METHOD_FIXED)
    {
      /* save ipv4 settings */

      dict = g_hash_table_new (g_str_hash, g_str_equal);
      if (method == METHOD_MANUAL)
        {
          char *address, *netmask, *gateway;

          if (!validate_static_ipv4_entries (item))
            {
               g_hash_table_destroy (dict);
               return;
            }

          address = (char*)gtk_entry_get_text (GTK_ENTRY (priv->address_entry));
          netmask = (char*)gtk_entry_get_text (GTK_ENTRY (priv->netmask_entry));
          gateway = (char*)gtk_entry_get_text (GTK_ENTRY (priv->gateway_entry));

          g_hash_table_insert (dict, "Method", "manual");
          g_hash_table_insert (dict, "Address", address);
          g_hash_table_insert (dict, "Netmask", netmask);
          g_hash_table_insert (dict, "Gateway", gateway);
        }
      else
        {
          g_hash_table_insert (dict, "Method", "dhcp");
        }

      value = g_new0 (GValue, 1);
      g_value_init (value, DBUS_TYPE_G_STRING_STRING_HASHTABLE);
      g_value_set_boxed (value, dict);

      net_connman_Service_set_property_async (priv->proxy,
                                              "IPv4.Configuration",
                                              value,
                                              set_configuration_cb,
                                              "IPv4.Configuration");
      g_free (value);
      g_hash_table_destroy (dict);
    }

  method = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->ipv6_method_combo));
  if (method != IPV6_METHOD_FIXED)
    {
      char *address, *gateway, *prefix;
      GValue val = {0};

      /* save ipv6 settings */

      dict = g_hash_table_new (g_str_hash, g_str_equal);
      switch (method)
      {
        case IPV6_METHOD_MANUAL:
          if (!validate_static_ipv6_entries (item))
            {
               g_hash_table_destroy (dict);
               return;
            }

          address = (char*)gtk_entry_get_text (GTK_ENTRY (priv->ipv6_address_entry));
          gateway = (char*)gtk_entry_get_text (GTK_ENTRY (priv->ipv6_gateway_entry));
          prefix = (char*)gtk_entry_get_text (GTK_ENTRY (priv->ipv6_prefix_length_entry));

          g_hash_table_insert (dict, "Method", "manual");
          g_hash_table_insert (dict, "Address", address);
          g_hash_table_insert (dict, "Gateway", gateway);
          if (strlen (prefix) > 0)
            {
              g_value_init (&val, G_TYPE_UCHAR);
              g_value_set_uchar (value, (unsigned char)strtoul (prefix, NULL, 10));
              g_hash_table_insert (dict, "PrefixLength", &value);
            }
          break;

        case IPV6_METHOD_AUTO:
          g_hash_table_insert (dict, "Method", "auto");
          break;
        case IPV6_METHOD_OFF:
          g_hash_table_insert (dict, "Method", "off");
          break;
        case IPV6_METHOD_FIXED:
          ; /* can't happen */
      }

      value = g_new0 (GValue, 1);
      g_value_init (value, DBUS_TYPE_G_STRING_STRING_HASHTABLE);
      g_value_set_boxed (value, dict);

      net_connman_Service_set_property_async (priv->proxy,
                                              "IPv6.Configuration",
                                              value,
                                              set_configuration_cb,
                                              "IPv6.Configuration");

      g_free (value);
      g_hash_table_destroy (dict);
    }
}


static void
carrick_service_item_save_dns_settings (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv;
  GValue *value;
  char **strv;

  priv = item->priv;
  value = g_new0 (GValue, 1);

  g_value_init (value, G_TYPE_STRV);
  strv = get_ip_addr_strv (GTK_TEXT_VIEW (priv->dns_text_view));
  g_value_set_boxed (value, strv);

  net_connman_Service_set_property_async (priv->proxy,
                                          "Nameservers.Configuration",
                                          value,
                                          set_nameservers_configuration_cb,
                                          item);

  g_free (value);
  g_strfreev (strv);
}

static void
apply_button_clicked_cb (GtkButton *button,
                         gpointer   user_data)
{
  CarrickServiceItem *item;

  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (user_data));
  item = CARRICK_SERVICE_ITEM (user_data);

  carrick_service_item_set_active (item, TRUE);

  if (!validate_dns_text_view (item))
    return;

  carrick_service_item_save_ip_settings (item);
  carrick_service_item_save_dns_settings (item);
  carrick_service_item_save_proxy_settings (item);

  /* start updating form again based on connman updates */
  _set_form_modified (item, FALSE);

  _unexpand_advanced_settings (item);
}

static GObject *
carrick_service_item_constructor (GType                  gtype,
                                  guint                  n_properties,
                                  GObjectConstructParam *properties)
{
  GObject            *obj;
  GObjectClass       *parent_class;
  CarrickServiceItemPrivate *priv;

  parent_class = G_OBJECT_CLASS (carrick_service_item_parent_class);
  obj = parent_class->constructor (gtype, n_properties, properties);
  priv = SERVICE_ITEM_PRIVATE (obj);

  if (!priv->model) {
    priv->is_modem_dummy = TRUE;
    priv->state = g_strdup ("idle");
    priv->strength = 0;
    carrick_service_item_set_type (CARRICK_SERVICE_ITEM (obj), "cellular");
 }
 gtk_widget_set_visible (priv->connect_box, !priv->is_modem_dummy);

  _set_state (CARRICK_SERVICE_ITEM (obj));

  return obj;
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
  object_class->constructor = carrick_service_item_constructor;

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

  pspec = g_param_spec_object ("ofono-agent",
                               "ofono-agent",
                               "CarrickOfonoAgent object",
                               CARRICK_TYPE_OFONO_AGENT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_OFONO_AGENT,
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

  pspec = g_param_spec_string ("type",
                               "type",
                               "Connman service type",
                               NULL,
                               G_PARAM_READABLE);
  g_object_class_install_property (object_class,
                                   PROP_TYPE,
                                   pspec);
}

static GtkWidget*
add_label_to_table (GtkTable *table, guint row, const char *text)
{
  GtkWidget *label;

  label = gtk_label_new (text);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD);
  gtk_widget_set_size_request (label, CARRICK_MIN_LABEL_WIDTH, -1);
  gtk_widget_show (label);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_misc_set_padding (GTK_MISC (label), 0, 5);
  gtk_table_attach (table, label,
                    0, 1, row, row + 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_SHRINK,
                    0, 0);

  return label;
}

static GtkWidget*
add_entry_to_table (GtkTable *table, guint row)
{
  GtkWidget *entry;

  entry = gtk_entry_new ();
  /* ipv4 mapped ipv6 address could be 45 chars */
  gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
  gtk_widget_show (entry);
  gtk_widget_set_size_request (entry, CARRICK_ENTRY_WIDTH, -1);
  gtk_table_attach (table, entry,
                    1, 2, row, row + 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);

  return entry;
}

static GtkWidget*
add_proxy_radio_to_table (GtkTable *table, guint row,
                          GtkWidget *group_widget, const char *text)
{
  GtkWidget *radio;
  GtkWidget *label;

  radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (group_widget),
                                                       text);
  /* hack to get a wrapping label */
  label = gtk_bin_get_child (GTK_BIN (radio));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD);
  /* set min width so the label can calculate how many rows it needs */
  gtk_widget_set_size_request (label, CARRICK_MIN_LABEL_WIDTH + CARRICK_ENTRY_WIDTH, -1);

  gtk_widget_show (radio);
  gtk_table_attach (table, radio,
                    0, 2, row, row + 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);
  return radio;
}

static void
carrick_service_item_init (CarrickServiceItem *self)
{
  CarrickServiceItemPrivate *priv;
  GtkWidget                 *box, *hbox, *vbox;
  GtkWidget                 *image;
  GtkWidget                 *modem_button;
  GtkWidget                 *table;
  GtkWidget                 *align;
  GtkWidget                 *sep;
  GtkWidget                 *label;
  GtkWidget                 *scrolled_window;
  GtkWidget                 *content_area;
  char                      *security_sample;
  guint                      row = 0;
  GtkSizeGroup              *group;

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

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
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
  gtk_misc_set_alignment (GTK_MISC (priv->icon), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (box),
                      priv->icon,
                      FALSE,
                      FALSE,
                      6);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (box),
                      vbox,
                      TRUE,
                      TRUE,
                      6);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox,
                      FALSE,
                      FALSE,
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

  priv->modem_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_visible (priv->modem_box, priv->is_modem_dummy);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->modem_box,
                      FALSE,
                      FALSE,
                      6);

  modem_button = gtk_button_new_with_label (_("Configure SIM card"));
  gtk_widget_show (modem_button);
  gtk_widget_set_size_request (modem_button, CARRICK_MIN_BUTTON_WIDTH, -1);
  g_signal_connect (modem_button,
                    "clicked",
                    G_CALLBACK (_modem_button_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (priv->modem_box),
                      modem_button,
                      FALSE,
                      FALSE,
                      6);


  priv->connect_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->connect_box,
                      FALSE,
                      FALSE,
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
  gtk_widget_set_size_request (priv->portal_button, CARRICK_MIN_BUTTON_WIDTH, -1);
  g_signal_connect (priv->portal_button, "clicked",
                    G_CALLBACK (_portal_button_cb), self);
  gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      priv->portal_button,
                      FALSE, FALSE, 6);

  priv->connect_button = gtk_button_new_with_label (_("Scanning"));
  gtk_widget_show (priv->connect_button);
  gtk_widget_set_size_request (priv->connect_button, CARRICK_MIN_BUTTON_WIDTH, -1);

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

  priv->passphrase_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), priv->passphrase_box,
                      FALSE, FALSE, 6);

  priv->passphrase_entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (priv->passphrase_entry), 20);
  gtk_entry_set_visibility (GTK_ENTRY (priv->passphrase_entry), TRUE);
  gtk_widget_show (priv->passphrase_entry);
  g_signal_connect (priv->passphrase_entry,
                    "activate",
                    G_CALLBACK (_passphrase_button_or_entry_cb),
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

  priv->passphrase_button = gtk_button_new ();
  gtk_widget_show (priv->passphrase_button);
  gtk_widget_set_size_request (priv->passphrase_button, CARRICK_MIN_BUTTON_WIDTH, -1);
  g_signal_connect (priv->passphrase_button,
                    "clicked",
                    G_CALLBACK (_passphrase_button_or_entry_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (priv->passphrase_box),
                      priv->passphrase_button,
                      FALSE, FALSE, 6);

  priv->show_password_check = gtk_check_button_new ();
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
  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  priv->advanced_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (priv->advanced_box);
  gtk_container_add (GTK_CONTAINER (priv->expando), priv->advanced_box);

  align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 6, 6, 6, 12);
  gtk_widget_show (align);
  gtk_box_pack_start (GTK_BOX (priv->advanced_box), align,
                      TRUE, TRUE, 0);


  table = gtk_table_new (7, 2, FALSE);
  gtk_size_group_add_widget (group, table);
  gtk_widget_show (table);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_container_add (GTK_CONTAINER (align), table);

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), _("<b>TCP/IP</b>"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label,
                    0, 2, row, row + 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);
  row++;

  /* TRANSLATORS: labels in advanced settings (next to combobox
   * for DHCP/Static IP and down from there) */
  add_label_to_table (GTK_TABLE (table), row++, _("Connect to IPv4:"));
  add_label_to_table (GTK_TABLE (table), row++, _("IP address:"));
  add_label_to_table (GTK_TABLE (table), row++, _("Subnet mask:"));
  add_label_to_table (GTK_TABLE (table), row++, _("Router:"));
  add_label_to_table (GTK_TABLE (table), row++, _("Connect to IPv6:"));
  add_label_to_table (GTK_TABLE (table), row++, _("IP address:"));
  add_label_to_table (GTK_TABLE (table), row++, _("Router:"));
  add_label_to_table (GTK_TABLE (table), row++, _("Prefix length:"));
  add_label_to_table (GTK_TABLE (table), row++, _("DNS:"));
  priv->mac_address_title_label =
    add_label_to_table (GTK_TABLE (table), row++, _("Your MAC address:"));
  gtk_widget_hide (priv->mac_address_title_label);

  priv->method_combo = gtk_combo_box_text_new ();
  /* NOTE: order/index of items in combobox is significant */
  /* TRANSLATORS: choices in the connection method combobox:
   * Will include "DHCP", "Static IP" and sometimes "Fixed IP" */
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (priv->method_combo),
                             METHOD_DHCP, _("DHCP"));
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (priv->method_combo),
                             METHOD_MANUAL, _("Static IP"));
  gtk_widget_set_size_request (priv->method_combo, CARRICK_ENTRY_WIDTH, -1);

  row = 1;
  gtk_widget_show (priv->method_combo);
  gtk_table_attach (GTK_TABLE (table), priv->method_combo,
                    1, 2, row, row + 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);
  g_signal_connect (priv->method_combo, "changed",
                    G_CALLBACK (method_combo_changed_cb), self);
  row++;

  priv->address_entry = add_entry_to_table (GTK_TABLE (table), row++);
  g_signal_connect (priv->address_entry, "notify::text",
                    G_CALLBACK (entry_notify_text), self);
  priv->netmask_entry = add_entry_to_table (GTK_TABLE (table), row++);
  g_signal_connect (priv->netmask_entry, "notify::text",
                    G_CALLBACK (entry_notify_text), self);
  priv->gateway_entry = add_entry_to_table (GTK_TABLE (table), row++);
  g_signal_connect (priv->gateway_entry, "notify::text",
                    G_CALLBACK (entry_notify_text), self);


  priv->ipv6_method_combo = gtk_combo_box_text_new ();
  /* NOTE: order/index of items in combobox is significant */
  /* TRANSLATORS: choices in the IPv6 connection method combobox:
   * Will include "Off", "Automatic", "Static IP" and sometimes "Fixed IP",
   * and possibly in future "DHCP" */
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (priv->ipv6_method_combo),
                             IPV6_METHOD_OFF, _("Off"));
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (priv->ipv6_method_combo),
                             IPV6_METHOD_AUTO, _("Automatic"));
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (priv->ipv6_method_combo),
                             IPV6_METHOD_MANUAL, _("Static IP"));
  gtk_widget_set_size_request (priv->ipv6_method_combo, CARRICK_ENTRY_WIDTH, -1);
  gtk_widget_show (priv->ipv6_method_combo);
  gtk_table_attach (GTK_TABLE (table), priv->ipv6_method_combo,
                    1, 2, row, row + 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);
  row++;
  g_signal_connect (priv->ipv6_method_combo, "changed",
                    G_CALLBACK (ipv6_method_combo_changed_cb), self);


  priv->ipv6_address_entry = add_entry_to_table (GTK_TABLE (table), row++);
  g_signal_connect (priv->ipv6_address_entry, "notify::text",
                    G_CALLBACK (entry_notify_text), self);
  priv->ipv6_prefix_length_entry = add_entry_to_table (GTK_TABLE (table), row++);
  g_signal_connect (priv->ipv6_prefix_length_entry, "notify::text",
                    G_CALLBACK (entry_notify_text), self);
  priv->ipv6_gateway_entry = add_entry_to_table (GTK_TABLE (table), row++);
  g_signal_connect (priv->ipv6_gateway_entry, "notify::text",
                    G_CALLBACK (entry_notify_text), self);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled_window);
  gtk_widget_set_size_request (scrolled_window, CARRICK_ENTRY_WIDTH, 39);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), scrolled_window,
                    1, 2, row, row + 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);
  row++;

  priv->dns_text_view = gtk_text_view_new ();
  gtk_widget_show (priv->dns_text_view);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->dns_text_view),
                               GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (priv->dns_text_view),
                                 FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->dns_text_view);
  g_signal_connect (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->dns_text_view)),
                    "changed",
                    G_CALLBACK (text_view_buffer_changed_cb),
                    self);

  priv->mac_address_label = gtk_label_new ("");
  gtk_label_set_selectable (GTK_LABEL (priv->mac_address_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (priv->mac_address_label), 0.0, 0.0);
  gtk_misc_set_padding (GTK_MISC (priv->mac_address_label), 0, 5);
  gtk_table_attach (GTK_TABLE (table), priv->mac_address_label,
                    1, 2, row, row + 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);
  row++;

  /* IP settings done */
  sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);

  gtk_widget_show (sep);
  gtk_box_pack_start (GTK_BOX (priv->advanced_box), sep,
                      FALSE, FALSE, 0);

  /* now, Proxy settings */

  align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 6, 6, 12, 6);
  gtk_widget_show (align);
  gtk_box_pack_start (GTK_BOX (priv->advanced_box), align,
                      TRUE, TRUE, 0);

  table = gtk_table_new (7, 2, FALSE);
  gtk_size_group_add_widget (group, table);
  gtk_widget_show (table);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_container_add (GTK_CONTAINER (align), table);

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), _("<b>Proxies</b>"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label,
                    0, 2, 0, 1,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);

  priv->proxy_none_radio = add_proxy_radio_to_table (
      GTK_TABLE (table), 1,
      NULL, _("No proxy"));
  g_signal_connect (priv->proxy_none_radio, "toggled",
                    G_CALLBACK (proxy_radio_toggled_cb), self);

  priv->proxy_auto_radio = add_proxy_radio_to_table (
      GTK_TABLE (table), 2,
      priv->proxy_none_radio, _("Auto-detect proxy settings for this network"));
  g_signal_connect (priv->proxy_auto_radio, "toggled",
                    G_CALLBACK (proxy_radio_toggled_cb), self);

  priv->proxy_url_radio = add_proxy_radio_to_table (
      GTK_TABLE (table), 3,
      priv->proxy_none_radio, _("Automatic proxy configuration URL:"));
  g_signal_connect (priv->proxy_url_radio, "toggled",
                    G_CALLBACK (proxy_radio_toggled_cb), self);

  priv->proxy_url_entry = gtk_entry_new ();
  gtk_widget_set_size_request (priv->proxy_url_entry, CARRICK_ENTRY_WIDTH, -1);
  gtk_table_attach (GTK_TABLE (table), priv->proxy_url_entry,
                    0, 2, 4, 5,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);
  g_signal_connect (priv->proxy_url_entry, "notify::text",
                    G_CALLBACK (entry_notify_text), self);

  priv->proxy_manual_radio = add_proxy_radio_to_table (
      GTK_TABLE (table), 5,
      priv->proxy_none_radio, _("Manual proxy server configuration:"));
  g_signal_connect (priv->proxy_manual_radio, "toggled",
                    G_CALLBACK (proxy_radio_toggled_cb), self);

  priv->proxy_manual_entry = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (priv->proxy_manual_entry),
                                  _("Server:Port"));
  gtk_widget_set_size_request (priv->proxy_manual_entry, CARRICK_ENTRY_WIDTH, -1);
  gtk_table_attach (GTK_TABLE (table), priv->proxy_manual_entry,
                    0, 2, 6, 7,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);
  g_signal_connect (priv->proxy_manual_entry, "notify::text",
                    G_CALLBACK (entry_notify_text), self);


  align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 12, 0, 0, 0);
  gtk_widget_show (align);
  gtk_table_attach (GTK_TABLE (table), align,
                    0, 2, 7, 8,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_SHRINK,
                    0, 0);

  label = gtk_label_new (_("Bypass proxy settings for these Hosts & Domains:"));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD);
  /* set min width so the label can calculate how many rows it needs */
  gtk_widget_set_size_request (label, CARRICK_MIN_LABEL_WIDTH + CARRICK_ENTRY_WIDTH + 20, -1);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (align), label);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled_window);
  gtk_widget_set_size_request (scrolled_window, CARRICK_ENTRY_WIDTH, 39);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), scrolled_window,
                    0, 2, 8, 9,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
                    0, 0);

  priv->proxy_bypass_text_view = gtk_text_view_new ();
  gtk_widget_show (priv->proxy_bypass_text_view);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->proxy_bypass_text_view),
                               GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (priv->proxy_bypass_text_view),
                                 FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->proxy_bypass_text_view);
  g_signal_connect (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->proxy_bypass_text_view)),
                    "changed",
                    G_CALLBACK (text_view_buffer_changed_cb),
                    self);

  align = gtk_alignment_new (1.0, 1.0, 0.0, 0.0);

  gtk_widget_show (align);
  gtk_table_attach (GTK_TABLE (table), align,
                    1, 2, 10, 11,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_EXPAND,
                    0, 0);

  /* TRANSLATORS: label for apply button in static ip & proxy settings */
  priv->apply_button = gtk_button_new_with_label (_("Apply"));
  gtk_widget_show (priv->apply_button);
  gtk_widget_set_size_request (priv->apply_button, CARRICK_MIN_BUTTON_WIDTH, -1);
  g_signal_connect (priv->apply_button, "clicked",
                    G_CALLBACK (apply_button_clicked_cb), self);
  gtk_container_add (GTK_CONTAINER (align), priv->apply_button);

  g_object_unref (group);
}

GtkWidget*
carrick_service_item_new (CarrickIconFactory         *icon_factory,
                          CarrickOfonoAgent          *ofono_agent,
                          CarrickNotificationManager *notifications,
                          CarrickNetworkModel        *model,
                          GtkTreePath                *path)
{
  return g_object_new (CARRICK_TYPE_SERVICE_ITEM,
                       "icon-factory", icon_factory,
                       "ofono-agent", ofono_agent,
                       "notification-manager", notifications,
                       "model", model,
                       "tree-path", path,
                       NULL);
}

GtkWidget*
carrick_service_item_new_as_modem_proxy (CarrickIconFactory         *icon_factory,
                                         CarrickOfonoAgent          *ofono_agent,
                                         CarrickNotificationManager *notifications)
{
  return g_object_new (CARRICK_TYPE_SERVICE_ITEM,
                       "icon-factory", icon_factory,
                       "ofono-agent", ofono_agent,
                       "notification-manager", notifications,
                       NULL);
}

const char*
carrick_service_item_get_service_type (CarrickServiceItem *item)
{
  return item->priv->type;
}

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
 * Written by - Joshua Lock <josh@linux.intel.com>
 *
 */

#include "carrick-network-model.h"

#include <config.h>
#include <dbus/dbus.h>

#include "connman-manager-bindings.h"
#include "connman-service-bindings.h"

G_DEFINE_TYPE (CarrickNetworkModel, carrick_network_model, GTK_TYPE_LIST_STORE)

#define NETWORK_MODEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_NETWORK_MODEL, CarrickNetworkModelPrivate))

struct _CarrickNetworkModelPrivate
{
  DBusGConnection *connection;
  DBusGProxy      *manager;
  GList           *services;
};

/*
 * Forward declaration of private methods
 */
static gint network_model_sort_cb (GtkTreeModel *self, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
static void network_model_manager_changed_cb (DBusGProxy *service, const gchar *property, GValue *value, gpointer user_data);
static void network_model_manager_get_properties_cb (DBusGProxy *manager, GHashTable *properties, GError *error, gpointer user_data);
static void carrick_network_model_dispose (GObject *object);
static gboolean network_model_have_service_by_path (GtkListStore *store, GtkTreeIter  *iter, const gchar  *path);
/* end */

static void
carrick_network_model_class_init (CarrickNetworkModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CarrickNetworkModelPrivate));

  object_class->dispose = carrick_network_model_dispose;
}

static void
carrick_network_model_init (CarrickNetworkModel *self)
{
  CarrickNetworkModelPrivate *priv;
  GError                     *error = NULL;

  priv = self->priv = NETWORK_MODEL_PRIVATE (self);
  priv->services = NULL;
  priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
  if (error)
    {
      g_debug ("Error connecting to bus: %s",
               error->message);
      g_clear_error (&error);
      /* FIXME: Do better? */
    }

  priv->manager = dbus_g_proxy_new_for_name (priv->connection,
                                             CONNMAN_SERVICE,
                                             CONNMAN_MANAGER_PATH,
                                             CONNMAN_MANAGER_INTERFACE);

  dbus_g_proxy_add_signal (priv->manager,
                           "PropertyChanged",
                           G_TYPE_STRING,
                           G_TYPE_VALUE,
                           G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (priv->manager,
                               "PropertyChanged",
                               G_CALLBACK (network_model_manager_changed_cb),
                               self,
                               NULL);

  net_connman_Manager_get_properties_async
    (priv->manager,
     network_model_manager_get_properties_cb,
     self);

  const GType column_types[] = { G_TYPE_OBJECT, /* proxy */
                                 G_TYPE_UINT, /* index */
                                 G_TYPE_STRING, /* name */
                                 G_TYPE_STRING, /* type */
                                 G_TYPE_STRING, /* state */
                                 G_TYPE_BOOLEAN, /* favourite */
                                 G_TYPE_UINT, /* strength */
                                 G_TYPE_STRING, /* security */
                                 G_TYPE_BOOLEAN, /* passphrase required */
                                 G_TYPE_STRING, /* passphrase */
                                 G_TYPE_BOOLEAN, /* setup required */
                                 G_TYPE_STRING, /* method */
                                 G_TYPE_STRING, /* address */
                                 G_TYPE_STRING, /* netmask */
                                 G_TYPE_STRING, /* gateway */
                                 G_TYPE_STRING, /* manually configured method */
                                 G_TYPE_STRING, /* manually configured address */
                                 G_TYPE_STRING, /* manually configured netmask */
                                 G_TYPE_STRING, /* manually configured gateway */
                                 G_TYPE_STRING, /* ipv6 method */
                                 G_TYPE_STRING, /* ipv6 address */
                                 G_TYPE_UINT, /* ipv6 prefix length */
                                 G_TYPE_STRING, /* ipv6 gateway */
                                 G_TYPE_STRING, /* manually configured ipv6 method */
                                 G_TYPE_STRING, /* manually configured ipv6 address */
                                 G_TYPE_UINT, /* manually configured ipv6 prefix length */
                                 G_TYPE_STRING, /* manually configured ipv6 gateway */
                                 G_TYPE_STRV, /* name servers */
                                 G_TYPE_STRV, /* manually configured name servers */
                                 G_TYPE_BOOLEAN, /* immutable */
                                 G_TYPE_BOOLEAN, /* login_required */
  };

  gtk_list_store_set_column_types (GTK_LIST_STORE (self),
                                   G_N_ELEMENTS (column_types),
                                   (GType *) column_types);

  gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (self),
                                           network_model_sort_cb,
                                           NULL, NULL);

  gtk_tree_sortable_set_sort_column_id
          (GTK_TREE_SORTABLE (self),
          GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
          GTK_SORT_DESCENDING);
}

static void
carrick_network_model_dispose (GObject *object)
{
  CarrickNetworkModel *self = CARRICK_NETWORK_MODEL (object);
  CarrickNetworkModelPrivate *priv = self->priv;
  GList *list_iter = NULL;
  gchar *path = NULL;
  GtkTreeIter iter;
  GtkListStore *store = GTK_LIST_STORE (self);

  if (priv->connection)
    {
      dbus_g_connection_unref (priv->connection);
      priv->connection = NULL;
    }

  for (list_iter = priv->services;
       list_iter != NULL;
       list_iter = list_iter->next)
    {
      path = list_iter->data;

      if (network_model_have_service_by_path (store, &iter, path) == TRUE)
        gtk_list_store_remove (store, &iter);

      g_free (path);
    }

  if (priv->services)
    {
      g_list_free (priv->services);
      priv->services = NULL;
    }

  G_OBJECT_CLASS (carrick_network_model_parent_class)->dispose(object);
}

static void
network_model_iterate_services (const GValue *value,
                                gpointer      user_data)
{
  GList **services = user_data;
  gchar  *path = g_value_dup_boxed (value);

  if (path != NULL)
    *services = g_list_append (*services, path);
}

static gboolean
network_model_have_service_in_store (GtkListStore *store,
                                     GtkTreeIter  *iter,
                                     const gchar  *path)
{
  gboolean    cont, found = FALSE;
  DBusGProxy *service;

  cont = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store),
                                        iter);

  while (cont)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store),
                          iter,
                          CARRICK_COLUMN_PROXY, &service,
                          -1);

      if (!service)
        break;

      found = g_str_equal (path,
                           dbus_g_proxy_get_path (service));
      g_object_unref (service);

      if (found)
        break;

      cont = gtk_tree_model_iter_next (GTK_TREE_MODEL (store),
                                       iter);
    }

  return found;
}

static gboolean
network_model_have_service_by_path (GtkListStore *store,
                                    GtkTreeIter  *iter,
                                    const gchar  *path)
{
  if (!path)
    return FALSE;

  return network_model_have_service_in_store (store, iter, path);
}

static gboolean
network_model_have_service_by_proxy (GtkListStore *store,
                                     GtkTreeIter  *iter,
                                     DBusGProxy   *service)
{
  const gchar *path;

  if (!service)
    return FALSE;

  path = dbus_g_proxy_get_path (service);

  return network_model_have_service_by_path (store, iter, path);
}

static const char*
get_string (GHashTable *properties, const char *prop_name)
{
  GValue *value;

  value = g_hash_table_lookup (properties, prop_name);
  if (value)
    return g_value_get_string (value);
  else
    return NULL;
}

static gboolean
get_boolean (GHashTable *properties, const char *prop_name)
{
  const GValue *value;

  value = g_hash_table_lookup (properties, prop_name);
  if (value)
    return g_value_get_boolean (value);
  else
    return FALSE;
}

static guint
get_uint (GHashTable *properties, const char *prop_name)
{
  const GValue *value;

  value = g_hash_table_lookup (properties, prop_name);
  if (value)
    return g_value_get_uchar (value);
  else
    return 0;
}

static gpointer
get_boxed (GHashTable *properties, const char *prop_name)
{
  GValue *value;

  value = g_hash_table_lookup (properties, prop_name);
  if (value)
    return g_value_get_boxed (value);
  else
    return NULL;
}

static void
network_model_service_get_properties_cb (DBusGProxy     *service,
                                         GHashTable     *properties,
                                         GError         *error,
                                         gpointer        user_data)
{
  CarrickNetworkModel *self = user_data;
  GtkListStore        *store = GTK_LIST_STORE (self);
  GHashTable          *dict;
  guint                strength;
  const gchar         *name, *type, *state, *security, *passphrase;
  gboolean             favorite, passphrase_required, setup_required,
                       immutable, login_required;

  const gchar         *method, *address, *netmask, *gateway;
  const gchar         *config_method, *config_address,
                      *config_netmask, *config_gateway;

  const gchar         *ipv6_method, *ipv6_address, *ipv6_gateway;
  const gchar         *config_ipv6_method, *config_ipv6_address,
                      *config_ipv6_gateway;
  guint                ipv6_prefix_length, config_ipv6_prefix_length;

  const gchar        **nameservers, **config_nameservers, **securities;
  GtkTreeIter          iter;

  if (error)
    {
      g_debug ("Couldn't end get properties call in _service_get_properties_cb - %s",
               error->message);
      g_error_free (error);
    }
  else
    {
      type = get_string (properties, "Type");
      state = get_string (properties, "State");
      favorite = get_boolean (properties, "Favorite");
      passphrase_required = get_boolean (properties, "PassphraseRequired");
      passphrase = get_string (properties, "Passphrase");
      setup_required = get_boolean (properties, "SetupRequired");
      nameservers = get_boxed (properties, "Nameservers");
      config_nameservers = get_boxed (properties, "Nameservers.Configuration");
      immutable = get_boolean (properties, "Immutable");
      login_required = get_boolean (properties, "LoginRequired");
      strength = get_uint (properties, "Strength");

      name = get_string (properties, "Name");
      if (!name)
        name = "";

      security = NULL;
      securities = get_boxed (properties, "Security");
      if (securities)
        {
          security = securities[0];

          while (security)
            {
	      if (g_strcmp0 ("none", security) == 0 ||
		  g_strcmp0 ("wep", security) == 0 ||
		  g_strcmp0 ("psk", security) == 0 ||
		  g_strcmp0 ("ieee8021x", security) == 0 ||
		  g_strcmp0 ("wpa", security) == 0 ||
		  g_strcmp0 ("rsn", security) == 0)
		      break;

	      security++;
	    }
        }

      method = address = netmask = gateway = NULL;
      dict = get_boxed (properties, "IPv4");
      if (dict)
        {
          method = get_string (dict, "Method");
          address = get_string (dict, "Address");
          netmask = get_string (dict, "Netmask");
          gateway = get_string (dict, "Gateway");
        }

      config_method = config_address = config_netmask = config_gateway = NULL;
      dict = get_boxed (properties, "IPv4.Configuration");
      if (dict)
        {
          config_method = get_string (dict, "Method");
          config_address = get_string (dict, "Address");
          config_netmask = get_string (dict, "Netmask");
          config_gateway = get_string (dict, "Gateway");
        }

      ipv6_method = ipv6_address = ipv6_gateway = NULL;
      ipv6_prefix_length = 0;
      dict = get_boxed (properties, "IPv6");
      if (dict)
        {
          ipv6_method = get_string (dict, "Method");
          ipv6_address = get_string (dict, "Address");
          ipv6_prefix_length = get_uint (dict, "PrefixLength");
          ipv6_gateway = get_string (dict, "Gateway");
        }

      config_ipv6_method = config_ipv6_address = config_ipv6_gateway = NULL;
      config_ipv6_prefix_length = 0;
      dict = get_boxed (properties, "IPv6.Configuration");
      if (dict)
        {
          config_ipv6_method = get_string (dict, "Method");
          config_ipv6_address = get_string (dict, "Address");
          config_ipv6_prefix_length = get_uint (dict, "PrefixLength");
          config_ipv6_gateway = get_string (dict, "Gateway");
        }

      if (network_model_have_service_by_proxy (store,
                                               &iter,
                                               service))
        {
          gtk_list_store_set (store, &iter,
                              CARRICK_COLUMN_NAME, name,
                              CARRICK_COLUMN_TYPE, type,
                              CARRICK_COLUMN_STATE, state,
                              CARRICK_COLUMN_FAVORITE, favorite,
                              CARRICK_COLUMN_STRENGTH, strength,
                              CARRICK_COLUMN_SECURITY, security,
                              CARRICK_COLUMN_PASSPHRASE_REQUIRED, passphrase_required,
                              CARRICK_COLUMN_PASSPHRASE, passphrase,
                              CARRICK_COLUMN_SETUP_REQUIRED, setup_required,
                              CARRICK_COLUMN_METHOD, method,
                              CARRICK_COLUMN_ADDRESS, address,
                              CARRICK_COLUMN_NETMASK, netmask,
                              CARRICK_COLUMN_GATEWAY, gateway,
                              CARRICK_COLUMN_CONFIGURED_METHOD, config_method,
                              CARRICK_COLUMN_CONFIGURED_ADDRESS, config_address,
                              CARRICK_COLUMN_CONFIGURED_NETMASK, config_netmask,
                              CARRICK_COLUMN_CONFIGURED_GATEWAY, config_gateway,
                              CARRICK_COLUMN_IPV6_METHOD, ipv6_method,
                              CARRICK_COLUMN_IPV6_ADDRESS, ipv6_address,
                              CARRICK_COLUMN_IPV6_PREFIX_LENGTH, ipv6_prefix_length,
                              CARRICK_COLUMN_IPV6_GATEWAY, ipv6_gateway,
                              CARRICK_COLUMN_CONFIGURED_IPV6_METHOD, config_ipv6_method,
                              CARRICK_COLUMN_CONFIGURED_IPV6_ADDRESS, config_ipv6_address,
                              CARRICK_COLUMN_CONFIGURED_IPV6_PREFIX_LENGTH, config_ipv6_prefix_length,
                              CARRICK_COLUMN_CONFIGURED_IPV6_GATEWAY, config_ipv6_gateway,
                              CARRICK_COLUMN_NAMESERVERS, nameservers,
                              CARRICK_COLUMN_CONFIGURED_NAMESERVERS, config_nameservers,
                              CARRICK_COLUMN_IMMUTABLE, immutable,
                              CARRICK_COLUMN_LOGIN_REQUIRED, login_required,
                              -1);
        }
      else
        {
          gtk_list_store_insert_with_values
            (store, &iter, -1,
             CARRICK_COLUMN_PROXY, service,
             CARRICK_COLUMN_NAME, name,
             CARRICK_COLUMN_TYPE, type,
             CARRICK_COLUMN_STATE, state,
             CARRICK_COLUMN_FAVORITE, favorite,
             CARRICK_COLUMN_STRENGTH, strength,
             CARRICK_COLUMN_SECURITY, security,
             CARRICK_COLUMN_PASSPHRASE_REQUIRED, passphrase,
             CARRICK_COLUMN_PASSPHRASE, passphrase,
             CARRICK_COLUMN_SETUP_REQUIRED, setup_required,
             CARRICK_COLUMN_METHOD, method,
             CARRICK_COLUMN_ADDRESS, address,
             CARRICK_COLUMN_NETMASK, netmask,
             CARRICK_COLUMN_GATEWAY, gateway,
             CARRICK_COLUMN_CONFIGURED_METHOD, config_method,
             CARRICK_COLUMN_CONFIGURED_ADDRESS, config_address,
             CARRICK_COLUMN_CONFIGURED_NETMASK, config_netmask,
             CARRICK_COLUMN_CONFIGURED_GATEWAY, config_gateway,
             CARRICK_COLUMN_IPV6_METHOD, ipv6_method,
             CARRICK_COLUMN_IPV6_ADDRESS, ipv6_address,
             CARRICK_COLUMN_IPV6_PREFIX_LENGTH, ipv6_prefix_length,
             CARRICK_COLUMN_IPV6_GATEWAY, ipv6_gateway,
             CARRICK_COLUMN_CONFIGURED_IPV6_METHOD, config_ipv6_method,
             CARRICK_COLUMN_CONFIGURED_IPV6_ADDRESS, config_ipv6_address,
             CARRICK_COLUMN_CONFIGURED_IPV6_PREFIX_LENGTH, config_ipv6_prefix_length,
             CARRICK_COLUMN_CONFIGURED_IPV6_GATEWAY, config_ipv6_gateway,
             CARRICK_COLUMN_NAMESERVERS, nameservers,
             CARRICK_COLUMN_CONFIGURED_NAMESERVERS, config_nameservers,
             CARRICK_COLUMN_IMMUTABLE, immutable,
             CARRICK_COLUMN_LOGIN_REQUIRED, login_required,
             -1);
        }
    }
}

static void
remove_provider_cb (DBusGProxy *proxy,
                    GError     *error,
                    gpointer    data)
{
  if (error)
    {
      g_debug ("Error on RemoveProvider: %s", error->message);
      g_error_free (error);
      return;
    }
}

static void
network_model_service_changed_cb (DBusGProxy  *service,
                                  const gchar *property,
                                  GValue      *value,
                                  gpointer     user_data)
{
  CarrickNetworkModel *self = user_data;
  GtkListStore        *store = GTK_LIST_STORE (self);
  GtkTreeIter          iter;
  GHashTable          *dict;

  if (property == NULL || value == NULL)
    return;

  if (network_model_have_service_by_proxy (store, &iter, service) == FALSE)
    return;

  if (g_str_equal (property, "State"))
    {
      const char *type, *state;
      DBusGProxy *service_proxy;

      /* HACK: connman (0.61) vpn handling is not consistent, so we
       * remove the provider on idle (otherwise it'll just hang there). 
       * But: set the state first, so notifications etc happen. */

      state = g_value_get_string (value);
      gtk_list_store_set (store, &iter,
                          CARRICK_COLUMN_STATE, state,
                          -1);

      gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                          CARRICK_COLUMN_TYPE, &type,
                          CARRICK_COLUMN_PROXY, &service_proxy,
                          -1);
      if (g_strcmp0 (type, "vpn") == 0 &&
          (g_strcmp0 (state, "idle") == 0 ||
           g_strcmp0 (state, "failure") == 0))
        net_connman_Manager_remove_provider_async (self->priv->manager,
                                                   dbus_g_proxy_get_path (service),
                                                   remove_provider_cb,
                                                   self);
    }
  else if (g_str_equal (property, "Favorite"))
    {
      gtk_list_store_set (store, &iter,
                          CARRICK_COLUMN_FAVORITE, g_value_get_boolean (value),
                          -1);
    }
  else if (g_str_equal (property, "Strength"))
    {
      gtk_list_store_set (store, &iter,
                          CARRICK_COLUMN_STRENGTH, g_value_get_uchar (value),
                          -1);
    }
  else if (g_str_equal (property, "Name"))
    {
      gtk_list_store_set (store, &iter,
                          CARRICK_COLUMN_NAME, g_value_get_string (value),
                          -1);
    }
  else if (g_str_equal (property, "PassphraseRequired") ||
	  g_str_equal (property, "SetupRequired"))
    {
      /* Rather than store this property we're just going to trigger
       * GetProperties to pull the up-to-date passphrase
       */
      net_connman_Service_get_properties_async
        (service,
         network_model_service_get_properties_cb,
         self);
    }
  else if (g_str_equal (property, "IPv4"))
    {
      dict = g_value_get_boxed (value);
      if (dict)
        {
          const char *method, *address, *netmask, *gateway;

          method = get_string (dict, "Method");
          address = get_string (dict, "Address");
          netmask = get_string (dict, "Netmask");
          gateway = get_string (dict, "Gateway");
          gtk_list_store_set (store, &iter,
                              CARRICK_COLUMN_METHOD, method,
                              CARRICK_COLUMN_ADDRESS, address,
                              CARRICK_COLUMN_NETMASK, netmask,
                              CARRICK_COLUMN_GATEWAY, gateway,
                              -1);
        }
    }
  else if (g_str_equal (property, "IPv4.Configuration"))
    {
      dict = g_value_get_boxed (value);
      if (dict)
        {
          const char *method, *address, *netmask, *gateway;

          method = get_string (dict, "Method");
          address = get_string (dict, "Address");
          netmask = get_string (dict, "Netmask");
          gateway = get_string (dict, "Gateway");
          gtk_list_store_set (store, &iter,
                              CARRICK_COLUMN_CONFIGURED_METHOD, method,
                              CARRICK_COLUMN_CONFIGURED_ADDRESS, address,
                              CARRICK_COLUMN_CONFIGURED_NETMASK, netmask,
                              CARRICK_COLUMN_CONFIGURED_GATEWAY, gateway,
                              -1);
        }
    }
  else if (g_str_equal (property, "IPv6"))
    {
      dict = g_value_get_boxed (value);
      if (dict)
        {
          const char *method, *address, *gateway;
          guint prefix;

          method = get_string (dict, "Method");
          address = get_string (dict, "Address");
          prefix = get_uint (dict, "PrefixLength");
          gateway = get_string (dict, "Gateway");
          gtk_list_store_set (store, &iter,
                              CARRICK_COLUMN_IPV6_METHOD, method,
                              CARRICK_COLUMN_IPV6_ADDRESS, address,
                              CARRICK_COLUMN_IPV6_PREFIX_LENGTH, prefix,
                              CARRICK_COLUMN_IPV6_GATEWAY, gateway,
                              -1);
        }
    }
  else if (g_str_equal (property, "IPv6.Configuration"))
    {
      dict = g_value_get_boxed (value);
      if (dict)
        {
          const char *method, *address, *gateway;
          guint prefix;

          method = get_string (dict, "Method");
          address = get_string (dict, "Address");
          prefix = get_uint (dict, "PrefixLength");
          gateway = get_string (dict, "Gateway");
          gtk_list_store_set (store, &iter,
                              CARRICK_COLUMN_CONFIGURED_IPV6_METHOD, method,
                              CARRICK_COLUMN_CONFIGURED_IPV6_ADDRESS, address,
                              CARRICK_COLUMN_CONFIGURED_IPV6_PREFIX_LENGTH, prefix,
                              CARRICK_COLUMN_CONFIGURED_IPV6_GATEWAY, gateway,
                              -1);
        }
    }
  else if (g_str_equal (property, "Nameservers"))
    {
      gtk_list_store_set (store, &iter,
                          CARRICK_COLUMN_NAMESERVERS,
                          g_value_get_boxed (value),
                          -1);
    }
  else if (g_str_equal (property, "Nameservers.Configuration"))
    {
      gtk_list_store_set (store, &iter,
                          CARRICK_COLUMN_CONFIGURED_NAMESERVERS,
                          g_value_get_boxed (value),
                          -1);
    }
  else if (g_str_equal (property, "Immutable"))
    {
      gtk_list_store_set (store, &iter,
                          CARRICK_COLUMN_IMMUTABLE, g_value_get_boolean (value),
                          -1);
    }
  else if (g_str_equal (property, "LoginRequired"))
    {
      gtk_list_store_set (store, &iter,
                          CARRICK_COLUMN_LOGIN_REQUIRED, g_value_get_boolean (value),
                          -1);
    }
}

static void
network_model_update_property (const gchar *property,
                               GValue      *value,
                               gpointer     user_data)
{
  CarrickNetworkModel        *self = user_data;
  CarrickNetworkModelPrivate *priv = self->priv;
  GtkListStore               *store = GTK_LIST_STORE (self);
  GList                      *new_services = NULL;
  GList                      *old_services = NULL;
  GList                      *list_iter = NULL;
  GList                      *tmp = NULL;
  GtkTreeIter                 iter;
  gchar                      *path = NULL;
  DBusGProxy                 *service;
  guint                       index = 0;

  if (g_str_equal (property, "Services"))
    {
      old_services = priv->services;

      dbus_g_type_collection_value_iterate (value,
                                            network_model_iterate_services,
                                            &new_services);

      priv->services = new_services;

      for (list_iter = new_services;
           list_iter != NULL;
           list_iter = list_iter->next)
        {
          path = list_iter->data;

          /* Remove from old list, if present.
           * We only want stale services in old list
           */
          tmp = g_list_find_custom (old_services,
                                    path,
                                    (GCompareFunc) g_strcmp0);
          if (tmp)
            {
              old_services = g_list_delete_link (old_services, tmp);
            }

          /* if we don't have the service in the model, add it*/
          if (network_model_have_service_by_path (store, &iter, path) == FALSE)
            {
              service = dbus_g_proxy_new_for_name (priv->connection,
                                                   CONNMAN_SERVICE, path,
                                                   CONNMAN_SERVICE_INTERFACE);

              gtk_list_store_insert_with_values (store, &iter, -1,
                                                 CARRICK_COLUMN_PROXY, service,
                                                 CARRICK_COLUMN_INDEX, index,
                                                 -1);

              dbus_g_proxy_add_signal (service,
                                       "PropertyChanged",
                                       G_TYPE_STRING,
                                       G_TYPE_VALUE,
                                       G_TYPE_INVALID);

              dbus_g_proxy_connect_signal (service,
                                           "PropertyChanged",
                                           G_CALLBACK
                                           (network_model_service_changed_cb),
                                           self,
                                           NULL);

              net_connman_Service_get_properties_async
                (service,
                 network_model_service_get_properties_cb,
                 self);

              g_object_unref (service);
            }
          /* else update it */
          else
            {
              gtk_list_store_set (store, &iter,
                                  CARRICK_COLUMN_INDEX, index,
                                  -1);
            }


          index++;
        }

      /* Old list only contains items not in new list.
       * Remove stale services */
      for (list_iter = old_services;
           list_iter != NULL;
           list_iter = list_iter->next)
        {
          path = list_iter->data;

          if (network_model_have_service_by_path (store, &iter, path) == TRUE)
            gtk_list_store_remove (store, &iter);

          g_free (path);
        }

      if (old_services)
        {
          g_list_free (old_services);
          old_services = NULL;
        }
    }
}

static gint
network_model_sort_cb (GtkTreeModel *model,
                       GtkTreeIter  *a,
                       GtkTreeIter  *b,
                       gpointer      user_data)
{
  guint ia, ib;

  gtk_tree_model_get (model, a, CARRICK_COLUMN_INDEX, &ia, -1);
  gtk_tree_model_get (model, b, CARRICK_COLUMN_INDEX, &ib, -1);

  return (gint)(ib - ia);
}

static void
network_model_manager_changed_cb (DBusGProxy  *proxy,
                                  const gchar *property,
                                  GValue      *value,
                                  gpointer     user_data)
{
  CarrickNetworkModel *self = user_data;

  network_model_update_property (property, value, self);
}

static void
network_model_manager_get_properties_cb (DBusGProxy     *manager,
                                         GHashTable     *properties,
                                         GError         *error,
                                         gpointer        user_data)
{
  CarrickNetworkModel *self = user_data;

  if (error)
    {
      g_debug ("Error: Couldn't end get properties call - %s",
               error->message);
      g_error_free (error);
      /* FIXME: Do something here */
    }
  else
    {
      g_hash_table_foreach (properties,
                            (GHFunc) network_model_update_property,
                            self);
      g_hash_table_unref (properties);
    }
}

/*
 * Public methods
 */

DBusGProxy *
carrick_network_model_get_proxy (CarrickNetworkModel *model)
{
  CarrickNetworkModelPrivate *priv = model->priv;

  return priv->manager;
}

GtkTreeModel *
carrick_network_model_new (void)
{
  return g_object_new (CARRICK_TYPE_NETWORK_MODEL,
                       NULL);
}

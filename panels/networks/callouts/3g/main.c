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
 * Written by - Ross Burton <ross@linux.intel.com>
 */

#include <config.h>
#include <stdlib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <rest/rest-xml-parser.h>
#include <gtk/gtk.h>

#include "ggg-service.h"
#include "ggg-mobile-info.h"
#include "ggg-country-dialog.h"
#include "ggg-service-dialog.h"
#include "ggg-provider-dialog.h"
#include "ggg-plan-dialog.h"
#include "ggg-manual-dialog.h"

#include "carrick/connman-manager-bindings.h"

#define CONNMAN_SERVICE           "net.connman"
#define CONNMAN_MANAGER_PATH      "/"
#define CONNMAN_MANAGER_INTERFACE CONNMAN_SERVICE ".Manager"
#define CONNMAN_SERVICE_INTERFACE CONNMAN_SERVICE ".Service"

static DBusGConnection *connection = NULL;

static char **service_paths = NULL;
static gboolean add_fake = FALSE;

static const GOptionEntry entries[] = {
  { "service", 's', 0, G_OPTION_ARG_STRING_ARRAY, &service_paths, "Service path", "PATH" },
  { "fake-service", 'f', 0, G_OPTION_ARG_NONE, &add_fake, "Add a fake service" },
  { NULL }
};

static enum {
  STATE_START,
  STATE_SERVICE,
  STATE_COUNTRY,
  STATE_PROVIDER,
  STATE_PLAN,
  STATE_MANUAL,
  STATE_SAVE,
  STATE_FINISH,
} state;

static GList *services = NULL;
static GggService *service;
static RestXmlNode *country_node = NULL;
static RestXmlNode *provider_node = NULL;
static RestXmlNode *plan_node = NULL;

static const char *
get_child_content (RestXmlNode *node, const char *name)
{
  g_assert (node);
  g_assert (name);

  node = rest_xml_node_find (node, name);
  if (node && node->content != '\0')
    return node->content;
  else
    return NULL;
}

static void
state_machine (void)
{
  GtkWidget *dialog;
  int old_state;

  while (state != STATE_FINISH) {
    /* For sanity checking state changes later */
    old_state = state;

    switch (state) {
    case STATE_START:
      /*
       * Determine if we need to select a service or can go straight to probing
       * the service.
       */
      if (services) {
        if (services->next) {
          dialog = g_object_new (GGG_TYPE_SERVICE_DIALOG,
                                 "services", services,
                                 NULL);
          switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
          case GTK_RESPONSE_CANCEL:
          case GTK_RESPONSE_DELETE_EVENT:
            state = STATE_FINISH;
            break;
          case GTK_RESPONSE_ACCEPT:
            service = ggg_service_dialog_get_selected (GGG_SERVICE_DIALOG (dialog));
            state = STATE_SERVICE;
            break;
          }
          gtk_widget_destroy (dialog);
        } else {
          service = services->data;
          state = STATE_SERVICE;
        }
      } else {
        g_printerr ("No services found\n");
        state = STATE_FINISH;
      }
      break;
    case STATE_SERVICE:
      /*
       * Exmaine the service and determine if we can probe some information, or
       * have to do guided configuration.
       */
      g_assert (service);

      if (ggg_service_is_roaming (service)) {
        state = STATE_COUNTRY;
      } else {
        provider_node = ggg_mobile_info_get_provider_for_ids
          (ggg_service_get_mcc (service), ggg_service_get_mnc (service));
        /* If we found a provider switch straight to the plan selector, otherwises
           fall back to the manual configuration */
        state = provider_node ? STATE_PLAN : STATE_MANUAL;
      }
      break;
    case STATE_COUNTRY:
      dialog = g_object_new (GGG_TYPE_COUNTRY_DIALOG, NULL);
      switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_DELETE_EVENT:
        state = STATE_FINISH;
        break;
      case GTK_RESPONSE_REJECT:
        state = STATE_MANUAL;
        break;
      case GTK_RESPONSE_ACCEPT:
        country_node = ggg_country_dialog_get_selected (GGG_COUNTRY_DIALOG (dialog));
        state = STATE_PROVIDER;
        break;
      }
      gtk_widget_destroy (dialog);
      break;
    case STATE_PROVIDER:
      g_assert (country_node);
      dialog = g_object_new (GGG_TYPE_PROVIDER_DIALOG,
                             "country", country_node,
                             NULL);
      switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_DELETE_EVENT:
        state = STATE_FINISH;
        break;
      case GTK_RESPONSE_REJECT:
        state = STATE_MANUAL;
        break;
      case GTK_RESPONSE_ACCEPT:
        provider_node = ggg_provider_dialog_get_selected (GGG_PROVIDER_DIALOG (dialog));
        state = STATE_PLAN;
        break;
      }
      gtk_widget_destroy (dialog);
      break;
    case STATE_PLAN:
      g_assert (provider_node);
      dialog = g_object_new (GGG_TYPE_PLAN_DIALOG,
                             "provider", provider_node,
                             NULL);
      switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_DELETE_EVENT:
        state = STATE_FINISH;
        break;
      case GTK_RESPONSE_REJECT:
        state = STATE_MANUAL;
        break;
      case GTK_RESPONSE_ACCEPT:
        plan_node = ggg_plan_dialog_get_selected (GGG_PLAN_DIALOG (dialog));
        state = STATE_SAVE;
        break;
      }
      gtk_widget_destroy (dialog);
      break;
    case STATE_MANUAL:
      dialog = g_object_new (GGG_TYPE_MANUAL_DIALOG, NULL);
      switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_DELETE_EVENT:
        state = STATE_FINISH;
        break;
      case GTK_RESPONSE_ACCEPT:
        plan_node = ggg_manual_dialog_get_plan (GGG_MANUAL_DIALOG (dialog));
        state = STATE_SAVE;
        break;
      }
      break;
    case STATE_SAVE:
      g_assert (plan_node);

      ggg_service_set (service,
                       rest_xml_node_get_attr (plan_node, "value"),
                       get_child_content (plan_node, "username"),
                       get_child_content (plan_node, "password"));

      ggg_service_connect (service);

      state = STATE_FINISH;
      break;
    case STATE_FINISH:
      break;
    }

    g_assert (state != old_state);
  }
}

static void
add_service (const GValue *value, gpointer user_data)
{
  GList **services = user_data;
  const char *path = g_value_get_boxed (value);
  GggService *service;

  if (path) {
    service = ggg_service_new (connection, path);
    if (service) {
      *services = g_list_append (*services, service);
    }
  }
}

static void
find_services (void)
{
  DBusGProxy *proxy;
  GHashTable *props = NULL;
  GError *error = NULL;
  GValue *value;

  proxy = dbus_g_proxy_new_for_name (connection,
                                     CONNMAN_SERVICE, "/",
                                     CONNMAN_MANAGER_INTERFACE);

  if (!net_connman_Manager_get_properties (proxy, &props, &error)) {
    g_printerr ("Cannot get properties for manager: %s\n", error->message);
    g_error_free (error);
    g_object_unref (proxy);
    return;
  }

  value = g_hash_table_lookup (props, "Services");
  if (value)
    dbus_g_type_collection_value_iterate (value, add_service, &services);

  g_hash_table_unref (props);
  g_object_unref (proxy);
}

static void
show_network_panel (void)
{
  DBusGConnection *session_bus;
  GError *error = NULL;
  DBusGProxy *proxy;

  session_bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (connection == NULL) {
    g_printerr ("Cannot connect to DBus: %s\n", error->message);
    g_error_free (error);
    return;
  }

  proxy = dbus_g_proxy_new_for_name (session_bus,
                                     "com.meego.UX.Shell.Toolbar",
                                     "/com/meego/UX/Shell/Toolbar",
                                     "com.meego.UX.Shell.Toolbar");

  dbus_g_proxy_call_no_reply (proxy, "ShowPanel",
                              G_TYPE_STRING, "network", G_TYPE_INVALID);
  /* Need to flush because we're out of the main loop by now */
  dbus_g_connection_flush (connection);

  g_object_unref (proxy);
}

int
main (int argc, char **argv)
{
  GError *error = NULL;
  GOptionContext *context;
  char **l;

  context = g_option_context_new ("- Carrick/ConnMan 3G connection wizard");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("option parsing failed: %s\n", error->message);
    exit (1);
  }

  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
  if (connection == NULL) {
    g_printerr ("Cannot connect to DBus: %s\n", error->message);
    g_error_free (error);
    exit (1);
  }

  if (service_paths) {
    for (l = service_paths; *l; l++) {
      GggService *service;

      service = ggg_service_new (connection, *l);
      if (service)
        services = g_list_prepend (services, service);
    }
  }

  if (add_fake) {
    services = g_list_prepend (services, ggg_service_new_fake ("Fake Device 1", FALSE));
    services = g_list_prepend (services, ggg_service_new_fake ("Fake Device 2", TRUE));
  }

  /* Scan connman for services if none were found */
  if (services == NULL)
    find_services ();

  gtk_window_set_default_icon_name ("network-wireless");

  state = STATE_START;
  state_machine ();

  show_network_panel ();

  return 0;
}

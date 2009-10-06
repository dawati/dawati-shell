#include <config.h>
#include <stdlib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <rest/rest-xml-parser.h>
#include <gtk/gtk.h>

#include "ggg-service.h"
#include "ggg-mobile-info.h"
#include "ggg-country-dialog.h"
#include "ggg-provider-dialog.h"
#include "ggg-plan-dialog.h"

#define CONNMAN_SERVICE           "org.moblin.connman"
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
  STATE_DONE
} state;

static GList *services = NULL;
static GggService *service;
static RestXmlNode *country_node = NULL;
static RestXmlNode *provider_node = NULL;
static RestXmlNode *plan_node = NULL;
static struct {
  char *apn;
  char *username;
  char *password;
} auth_data = { NULL, NULL, NULL };

static void
state_machine (void)
{
  GtkWidget *dialog;

  while (state != STATE_DONE) {
    switch (state) {
    case STATE_START:
      /*
       * Determine if we need to select a service or can go straight to probing
       * the service.
       */
      if (services) {
        if (services->next) {
          /* TODO switch to select service state */
        } else {
          service = services->data;
          state = STATE_SERVICE;
        }
      } else {
        /* TODO: error dialog */
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
    case STATE_COUNTRY:
      dialog = g_object_new (GGG_TYPE_COUNTRY_DIALOG, NULL);
      switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_DELETE_EVENT:
        state = STATE_DONE;
        break;
      case GTK_RESPONSE_REJECT:
        state = STATE_MANUAL;
        break;
      case GTK_RESPONSE_ACCEPT:
        country_node = ggg_country_dialog_get_selected (GGG_COUNTRY_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        state = STATE_PROVIDER;
        break;
      }
      break;
    case STATE_PROVIDER:
      g_assert (country_node);
      dialog = g_object_new (GGG_TYPE_PROVIDER_DIALOG,
                             "country", country_node,
                             NULL);
      switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_DELETE_EVENT:
        state = STATE_DONE;
        break;
      case GTK_RESPONSE_REJECT:
        state = STATE_MANUAL;
        break;
      case GTK_RESPONSE_ACCEPT:
        provider_node = ggg_provider_dialog_get_selected (GGG_PROVIDER_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        state = STATE_PLAN;
        break;
      }
      break;
    case STATE_PLAN:
      g_assert (provider_node);
      dialog = g_object_new (GGG_TYPE_PLAN_DIALOG,
                             "provider", provider_node,
                             NULL);
      switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_DELETE_EVENT:
        state = STATE_DONE;
        break;
      case GTK_RESPONSE_REJECT:
        state = STATE_MANUAL;
        break;
      case GTK_RESPONSE_ACCEPT:
        plan_node = ggg_plan_dialog_get_selected (GGG_PLAN_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        g_assert (plan_node);
        /* TOOD: not shit */
        auth_data.apn = rest_xml_node_get_attr (plan_node, "value");
        auth_data.username = rest_xml_node_find (plan_node, "username")->content;
        auth_data.password = rest_xml_node_find (plan_node, "password")->content;
        state = STATE_SAVE;
        break;
      }
      break;
    case STATE_MANUAL:
      /* TODO */
      state = STATE_DONE;
      break;
    case STATE_SAVE:
      if (auth_data.apn) {
        ggg_service_set (service,
                         auth_data.apn,
                         auth_data.username,
                         auth_data.password);
      }
      /* TODO: write data */
      state = STATE_DONE;
      break;
    case STATE_DONE:
      break;
    }
  }
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
    services = g_list_prepend (services, ggg_service_new_fake ());
  }

  /* TODO: scan connman for services if none were found */
  /* TODO: handle multiple services */

  gtk_window_set_default_icon_name ("network-wireless");

  state = STATE_START;
  state_machine ();

  return 0;
}

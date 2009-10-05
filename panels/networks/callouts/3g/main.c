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

static void
configure_roaming_service (GggService *service)
{
  GtkWidget *dialog;
  RestXmlNode *node;

  dialog = g_object_new (GGG_TYPE_COUNTRY_DIALOG, NULL);
  switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
  case GTK_RESPONSE_CANCEL:
    /* Cancel button, do nothing */
    break;
  case GTK_RESPONSE_REJECT:
    /* Manual button */
    /* TODO */
    break;
  case GTK_RESPONSE_ACCEPT:
    /* OK button */
    node = ggg_country_dialog_get_selected (GGG_COUNTRY_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    dialog = g_object_new (GGG_TYPE_PROVIDER_DIALOG,
                           "country", node, NULL);
    switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
    case GTK_RESPONSE_CANCEL:
      break;
    case GTK_RESPONSE_REJECT:
      /* TODO */
      break;
    case GTK_RESPONSE_ACCEPT:
      node = ggg_provider_dialog_get_selected (GGG_PROVIDER_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      dialog = g_object_new (GGG_TYPE_PLAN_DIALOG,
                             "provider", node, NULL);
      gtk_dialog_run (GTK_DIALOG (dialog));
    }
    break;
  }
}

static void
configure_service (GggService *service)
{
  RestXmlNode *provider;
  GtkWidget *dialog;

  provider = ggg_mobile_info_get_provider_for_ids
    (ggg_service_get_mcc (service), ggg_service_get_mnc (service));

  if (provider) {
    dialog = g_object_new (GGG_TYPE_PLAN_DIALOG,
                           "provider", provider, NULL);
    gtk_dialog_run (GTK_DIALOG (dialog));
  } else {
    /* TODO: not roaming but manual configuration */
    configure_roaming_service (service);
  }
}

static void
configure (GggService *service)
{
  if (ggg_service_is_roaming (service)) {
    configure_roaming_service (service);
  } else {
    configure_service (service);
  }
}

int
main (int argc, char **argv)
{
  GError *error = NULL;
  GOptionContext *context;
  GList *services = NULL;
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

  configure (services->data);

  return 0;
}

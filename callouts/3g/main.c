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
 * Written by - Ross Burton <ross@linux.intel.com>
 *              Jussi Kukkonen <jussi.kukkonen@intel.com>
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>

#include <rest/rest-xml-parser.h>
#include <gtk/gtk.h>

#include "ggg-mobile-info.h"
#include "ggg-country-dialog.h"
#include "ggg-provider-dialog.h"
#include "ggg-plan-dialog.h"
#include "ggg-manual-dialog.h"
#include "ggg-sim-dialog.h"
#include "ggg-sim.h"


static gboolean add_fake = FALSE;

static GHashTable *sims;

static const GOptionEntry entries[] = {
  { "fake-service", 'f', 0, G_OPTION_ARG_NONE, &add_fake, "Add a fake service" },
  { NULL }
};

static enum {
  STATE_START,
  STATE_BROADBAND_INFO,
  STATE_COUNTRY,
  STATE_PROVIDER,
  STATE_PLAN,
  STATE_MANUAL,
  STATE_SAVE,
  STATE_FINISH,
} state;

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
save_context (Sim *sim, const char *name)
{
  const char *username, *password, *apn;

  apn = rest_xml_node_get_attr (plan_node, "value");
  if (!apn)
    apn = "";
  username = get_child_content (plan_node, "username");
  if (!username)
    username = "";
  password = get_child_content (plan_node, "password");
  if (!password)
    password = "";

  sim_set_plan (sim, name, apn, username, password);
}

static void
state_machine (void)
{
  GtkWidget *dialog;
  int old_state;
  Sim *sim = NULL;
  char *name, *apn, *username, *password;
  const char *provider;

  name = apn = username = password = NULL;

  while (state != STATE_FINISH) {
    /* For sanity checking state changes later */
    old_state = state;

    switch (state) {
    case STATE_START:
      g_assert (g_hash_table_size (sims) > 0);

      /* Select modem/sim if have several */
      if (g_hash_table_size (sims) == 1) {
        GHashTableIter iter;
        g_hash_table_iter_init (&iter, sims);
        g_hash_table_iter_next (&iter, NULL, (gpointer)&sim);
        state = STATE_BROADBAND_INFO;
      } else {
        dialog = g_object_new (GGG_TYPE_SIM_DIALOG,
                               "sims", sims,
                               NULL);
        switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_DELETE_EVENT:
          state = STATE_FINISH;
          break;
        case GTK_RESPONSE_ACCEPT:
          sim = ggg_sim_dialog_get_selected (GGG_SIM_DIALOG (dialog));
          state = STATE_BROADBAND_INFO;
        break;
        }
        gtk_widget_destroy (dialog);
      }
      break;

     case STATE_BROADBAND_INFO:
      /* Go to plan, provider, country or manual selection depending on what we know
       * from mobile-broadband-info and ofono */
      g_assert (sim);

      name = apn = username = password = NULL;

      provider_node = ggg_mobile_info_get_provider_for_ids (sim->mcc, sim->mnc);
      if (provider_node) {
        state = STATE_PLAN;
      } else {
        country_node = ggg_mobile_info_get_country_for_mcc (sim->mcc);
        state = country_node ? STATE_PROVIDER : STATE_COUNTRY;
      }

      if (sim_get_plan (sim, &name, &apn, &username, &password)) {
        /* ofono has a context already. skip to showing current values
         * and let the user restart wizard if needed.
         * One exception: if all values are empty, consider this a
         * non-existing context (ofono may do this buggy behaviour).
         * */
        if ((!username || strlen (username) == 0) &&
            (!apn || strlen (apn) == 0) &&
            (!password || strlen (password) == 0))
          sim_remove_plan (sim);
        else
          state = STATE_MANUAL;
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
        /* guess a name for the context */
        provider = get_child_content (provider_node, "name");
        if (provider)
          name = g_strdup_printf ("%s Internet", provider);
        else
          name = g_strdup ("Internet");

        state = STATE_SAVE;
        break;
      }
      gtk_widget_destroy (dialog);
      break;

    case STATE_MANUAL:
      dialog = g_object_new (GGG_TYPE_MANUAL_DIALOG,
                             "name", name,
                             "apn", apn,
                             "username", username,
                             "password", password,
                             NULL);
      switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
      case GTK_RESPONSE_CANCEL:
      case GTK_RESPONSE_CLOSE:
      case GTK_RESPONSE_DELETE_EVENT:
        state = STATE_FINISH;
        break;
      case GTK_RESPONSE_REJECT:
        sim_remove_plan (sim);
        state = STATE_BROADBAND_INFO;
        break;
      case GTK_RESPONSE_ACCEPT:
        plan_node = ggg_manual_dialog_get_plan (GGG_MANUAL_DIALOG (dialog));
        state = STATE_SAVE;
        break;
      }
      gtk_widget_destroy (dialog);
      break;

    case STATE_SAVE:
      g_assert (plan_node);
      save_context (sim, name);
      state = STATE_FINISH;
      break;
    case STATE_FINISH:
      break;
    }

    g_assert (state != old_state);
  }
}

static void
find_sims ()
{
  GDBusProxy *proxy;
  GError *err = NULL;
  GVariant *modems;
  char *obj_path;
  GVariantIter *iter;
  
  /* TODO don't be synchronous... */
  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL, /* should add iface info here */
                                         OFONO_SERVICE,
                                         OFONO_MANAGER_PATH,
                                         OFONO_MANAGER_INTERFACE,
                                         NULL,
                                         &err);
  if (err) {
    g_warning ("No ofono proxy: %s", err->message);
    return;
  }

  modems = g_dbus_proxy_call_sync (proxy,
                                   "GetModems",
                                   NULL,
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &err);
  if (err) {
    g_warning ("GetModems failed: %s", err->message);
    return;
  }

  g_variant_get (modems, "(a(oa{sv}))", &iter);
  while (g_variant_iter_loop (iter, "(oa{sv})", &obj_path, NULL)) {
    Sim *sim = sim_new (obj_path);
    if (sim)
      g_hash_table_insert (sims, g_strdup (obj_path), sim);
  }
  g_variant_iter_free (iter);
  g_variant_unref (modems);
}

static void
show_network_panel (void)
{
  GError *error = NULL;
  GDBusProxy *proxy;

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL, /* should add iface info here */
                                         "com.dawati.UX.Shell.Toolbar",
                                         "/com/dawati/UX/Shell/Toolbar",
                                         "com.dawati.UX.Shell.Toolbar",
                                         NULL,
                                         &error);
  if (error) {
    g_warning ("No Toolbar proxy: %s", error->message);
    return;
  }

  g_dbus_proxy_call_sync (proxy,
                          "ShowPanel",
                          g_variant_new ("(s)", "network"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL, 
                          &error);
  if (error) {
    g_warning ("ShowPanel failed: %s", error->message);
  }

  g_object_unref (proxy);
}

int
main (int argc, char **argv)
{
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new ("- Dawati shell 3G connection wizard");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("option parsing failed: %s\n", error->message);
    exit (1);
  }

  sims = g_hash_table_new_full (g_str_hash, g_str_equal,
                                g_free, (GDestroyNotify)sim_free);
  find_sims ();
  if (g_hash_table_size (sims) == 0) {
    g_debug ("No Modem/Sim found");
    return 0;
  }

  gtk_window_set_default_icon_name ("network-wireless");

  state = STATE_START;
  state_machine ();

  show_network_panel ();

  return 0;
}

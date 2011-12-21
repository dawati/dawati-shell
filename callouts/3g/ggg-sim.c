/*
 * Carrick - a connection panel for the Dawati Netbook
 * Copyright (C) 2011 Intel Corporation. All rights reserved.
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
 * Written by - Jussi Kukkonen <jussi.kukkonen@intel.com>
 */

#include "ggg-sim.h"

void
sim_free (Sim *sim)
{
  g_free (sim->context_path);
  g_free (sim->mnc);
  g_free (sim->mcc);
  g_free (sim->device_name);
  g_object_unref (sim->modem_proxy);
  g_slice_free (Sim, sim);
}

Sim*
sim_new (const char *modem_path)
{
  GError *err = NULL;
  Sim *sim;
  GVariant *contexts, *props, *value;
  GVariantIter *iter, *obj_iter;
  gboolean has_connection_manager = FALSE;
  gboolean has_sim_manager = FALSE;
  char *key, *context_path, *model, *manufacturer;

  model = manufacturer = NULL;
  sim = g_slice_new0 (Sim);

  sim->modem_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                   NULL, /* should add iface info here */
                                                   OFONO_SERVICE,
                                                   modem_path,
                                                   OFONO_MODEM_INTERFACE,
                                                   NULL,
                                                   &err);
  if (err) {
    g_warning ("Could not create Modem proxy: %s", err->message);
    g_error_free (err);
    sim_free (sim);
    return NULL;
  }

  /* check if this modem supports the ifaces we need */
  props = g_dbus_proxy_call_sync (sim->modem_proxy,
                                  "org.ofono.Modem.GetProperties",
                                  NULL,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &err);
  if (err) {
    g_warning ("Modem.GetProperties failed: %s", err->message);
    g_error_free (err);
    sim_free (sim);
    return NULL;
  }

  g_variant_get (props, "(a{sv})", &iter);
  while (g_variant_iter_loop (iter, "{sv}", &key, &value)) {
    if (g_strcmp0 (key, "Interfaces") == 0) {
      const char **ifaces;
      ifaces = g_variant_get_strv (value, NULL);
      while (*ifaces) {
        if (g_strcmp0 (*ifaces, "org.ofono.ConnectionManager") == 0) {
          has_connection_manager = TRUE;
        } else if (g_strcmp0 (*ifaces, "org.ofono.SimManager") == 0) {
          has_sim_manager = TRUE;
        }
        ifaces++;
      }
    } else if (g_strcmp0 (key, "Manufacturer") == 0) {
      manufacturer = g_variant_get_string (value, NULL);
    } else if (g_strcmp0 (key, "Model") == 0) {
      model = g_variant_get_string (value, NULL);
    }
  }

  if (manufacturer && model)
    sim->device_name = g_strdup_printf ("%s %s", manufacturer, model);
  else if (manufacturer)
    sim->device_name = g_strdup (manufacturer);
  else if (model)
    sim->device_name = g_strdup (model);
  else
    sim->device_name = g_strdup (modem_path);
  
  g_variant_iter_free (iter);
  g_variant_unref (props);

  if (!has_connection_manager && !has_sim_manager) {
    sim_free (sim);
    return NULL;
  }
    
  /* find out if we already have a internet context */
  contexts = g_dbus_proxy_call_sync (sim->modem_proxy,
                                    "org.ofono.ConnectionManager.GetContexts",
                                     NULL,
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL,
                                     &err);
  if (err) {
    g_warning ("GetContexts failed: %s", err->message);
    g_error_free (err);
    sim_free (sim);
    return NULL;
  }

  g_variant_get (contexts, "(a(oa{sv}))", &iter);
  while (g_variant_iter_loop (iter, "(oa{sv})", &context_path, &obj_iter)) {
     while (g_variant_iter_loop (obj_iter, "{sv}", &key, &value)) {
       if (g_strcmp0 (key, "Type") == 0 &&
           g_strcmp0 (g_variant_get_string (value, NULL), "internet") == 0) {
         sim->context_path = g_strdup (context_path);
       }
    }
  }
  g_variant_iter_free (iter);
  g_variant_unref (contexts);
      
  /* see if we have MNC and MCC */
  props = g_dbus_proxy_call_sync (sim->modem_proxy,
                                  "org.ofono.SimManager.GetProperties",
                                  NULL,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &err);
  if (err) {
    g_warning ("GetProperties failed: %s", err->message);
    g_error_free (err);
    sim_free (sim);
    return NULL;
  }
  g_variant_get (props, "(a{sv})", &iter);
  while (g_variant_iter_loop (iter, "{sv}", &key, &value)) {
    if (g_strcmp0 (key, "MobileCountryCode") == 0)
      sim->mcc = g_strdup (g_variant_get_string (value, NULL));
    else if (g_strcmp0 (key, "MobileNetworkCode") == 0)
      sim->mnc = g_strdup (g_variant_get_string (value, NULL));
  }
  g_variant_iter_free (iter);
  g_variant_unref (props);

  return sim;
}

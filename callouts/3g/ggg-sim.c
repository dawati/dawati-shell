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
  g_free (sim->mnc);
  g_free (sim->mcc);
  g_free (sim->device_name);
  if (sim->modem_proxy)
    g_object_unref (sim->modem_proxy);
  if (sim->context_proxy)
    g_object_unref (sim->context_proxy);
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
  char *key, *context_path;
  const char *model, *manufacturer;

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

  if (!has_connection_manager || !has_sim_manager) {
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
        sim->context_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                            G_DBUS_PROXY_FLAGS_NONE,
                                                            NULL,
                                                            OFONO_SERVICE,
                                                            context_path,
                                                            OFONO_CONTEXT_INTERFACE,
                                                            NULL,
                                                            &err);
        if (err) {
          g_warning ("Failed to get context proxy: %s", err->message);
          g_error_free (err);
        }
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

static gboolean
set_property (GDBusProxy *proxy,
              const char *key,
              const GVariant *value)
{
  GError *err = NULL;

  g_dbus_proxy_call_sync (proxy,
                          "SetProperty",
                          g_variant_new ("(sv)", key, value),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          &err);
  if (err) {
    g_warning ("SetProperty failed: %s", err->message);
    g_error_free (err);
    return FALSE;
  } 

  return TRUE;
}

gboolean
sim_set_plan (Sim *sim,
              const char *name,
              const char *apn,
              const char *username,
              const char *password)
{
  GVariant *var;
  GError *err = NULL;
  const char *context_path;
  gboolean ok = TRUE;

  g_return_val_if_fail (sim, FALSE);
  g_return_val_if_fail (sim->modem_proxy, FALSE);
  g_return_val_if_fail (apn && username && password, FALSE);

  if (!sim->context_proxy) {
    var = g_dbus_proxy_call_sync (sim->modem_proxy,
                                  "org.ofono.ConnectionManager.AddContext",
                                  g_variant_new ("(s)", "internet"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &err);
    if (err) {
      g_warning ("AddContext failed: %s", err->message);
      g_error_free (err);
      return FALSE;
    } 
    g_variant_get (var, "(o)", &context_path);

    sim->context_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                        G_DBUS_PROXY_FLAGS_NONE,
                                                        NULL, /* should add iface info here */
                                                        OFONO_SERVICE,
                                                        context_path,
                                                        OFONO_CONTEXT_INTERFACE,
                                                        NULL,
                                                        &err);
    g_variant_unref (var);
    if (err) {
      g_warning ("Failed to get proxy: %s", err->message);
      g_error_free (err);
      /* TODO ? */
      return FALSE;
    } 
  }

  if (name)
    ok = set_property (sim->context_proxy,
                       "Name",
                       g_variant_new_string (name));
          
  ok = ok &&
       set_property (sim->context_proxy,
                     "AccessPointName",
                     g_variant_new_string (apn)) &&
       set_property (sim->context_proxy,
                     "Username",
                     g_variant_new_string (username)) &&
       set_property (sim->context_proxy,
                     "Password",
                     g_variant_new_string (password));
  return ok;
}

gboolean
sim_get_plan (Sim *sim, char **name, char **apn, char **username, char **password)
{
  GError *err = NULL;
  GVariantIter *iter;
  GVariant *properties, *value;
  char *key;

  g_return_val_if_fail (sim, FALSE);

  if (!sim->context_proxy)
    return FALSE;

  properties = g_dbus_proxy_call_sync (sim->context_proxy,
                                       "GetProperties",
                                       NULL,
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       NULL,
                                       &err);
  if (err) {
    g_warning ("ConnectionContext.GetProperties failed: %s", err->message);
    g_error_free (err);
    return FALSE;
  }

  g_variant_get (properties, "(a{sv})", &iter);
  while (g_variant_iter_loop (iter, "{sv}", &key, &value)) {
    if (name && g_strcmp0 (key, "Name") == 0)
      *name = g_strdup (g_variant_get_string (value, NULL));
    if (apn && g_strcmp0 (key, "AccessPointName") == 0)
      *apn = g_strdup (g_variant_get_string (value, NULL));
    else if (username && g_strcmp0 (key, "Username") == 0)
      *username = g_strdup (g_variant_get_string (value, NULL));
    if (password && g_strcmp0 (key, "Password") == 0)
      *password = g_strdup (g_variant_get_string (value, NULL));
  }

  g_variant_iter_free (iter);
  g_variant_unref (properties);

  return TRUE;
}

gboolean
sim_remove_plan (Sim *sim)
{
  GError *err = NULL;

  g_return_val_if_fail (sim, FALSE);
  g_return_val_if_fail (sim->modem_proxy, FALSE);
  g_return_val_if_fail (sim->context_proxy, FALSE);

  g_dbus_proxy_call_sync (sim->modem_proxy,
                          "org.ofono.ConnectionManager.RemoveContext",
                          g_variant_new ("(o)", g_dbus_proxy_get_object_path (sim->context_proxy)),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          &err);
  if (err) {
    g_warning ("RemoveContext failed: %s", err->message);
    g_error_free (err);
    return FALSE;
  }

  g_object_unref (sim->context_proxy);
  sim->context_proxy = NULL;
  return TRUE;
}

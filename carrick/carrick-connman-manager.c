/*
 * Carrick - a connection panel for the Dawati Netbook
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Author Jussi Kukkonen <jussi.kukkonen@intel.com>
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
 */

/* This object is not actually used in carrick-the-connection-panel
 * at the moment, but is used by other panels -- it provides easy access
 * to a subset of Connman manager state. Carrick should move to
 * it in future as well.
 * */

#include <gio/gio.h>

#include "carrick-connman-manager.h"

G_DEFINE_TYPE (CarrickConnmanManager, carrick_connman_manager, G_TYPE_OBJECT)

#define CONNMAN_MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_CONNMAN_MANAGER, CarrickConnmanManagerPrivate))

struct _CarrickConnmanManagerPrivate
{
  GDBusProxy *cm;

  char **enabled_techs;
  char **available_techs;
};

enum
{
  PROP_0,
  PROP_AVAILABLE_TECHNOLOGIES,
  PROP_ENABLED_TECHNOLOGIES,
};

static void carrick_connman_manager_handle_property (CarrickConnmanManager *manager, const char *key, GVariant *value);
static void carrick_connman_manager_set_available_techs (CarrickConnmanManager *self, const char **techs);
static void carrick_connman_manager_set_enabled_techs (CarrickConnmanManager *self, const char **techs);

static void
_connman_get_properties_cb (GDBusProxy            *cm,
                            GAsyncResult          *res,
                            CarrickConnmanManager *self)
{
  GError *err = NULL;
  GVariant *props, *value;
  GVariantIter *iter;
  const char *key;

  props = g_dbus_proxy_call_finish (cm, res, &err);
  if (err) {
    g_warning ("Connman GetProperties failed: %s", err->message);
    g_error_free (err);
    return;
  }

  g_variant_get (props, "(a{sv})", &iter);
  while (g_variant_iter_loop (iter, "{sv}", &key, &value))
    carrick_connman_manager_handle_property (self, key, value);

  g_variant_iter_free (iter);
  g_variant_unref (props);
}

static void
_connman_signal_cb (GDBusProxy            *cm,
                    gchar                 *sender_name,
                    gchar                 *signal_name,
                    GVariant              *parameters,
                    CarrickConnmanManager *self)
{
  GVariant *value;
  const char *key;

  if (g_strcmp0 (signal_name, "PropertyChanged") == 0) {
     g_variant_get (parameters, "(sv)", &key, &value);
     carrick_connman_manager_handle_property (self, key, value);
  }
}

static void
_connman_proxy_new_for_bus_cb (GObject               *object,
                               GAsyncResult          *res,
                               CarrickConnmanManager *self)
{
  GError *err = NULL;

  if (self->priv->cm) {
    g_signal_handlers_disconnect_by_func (self->priv->cm,
                                          G_CALLBACK (_connman_signal_cb),
                                          self);
    g_object_unref (self->priv->cm);
  }

  self->priv->cm = g_dbus_proxy_new_for_bus_finish (res, &err);
  if (err) {
    g_warning ("No connman proxy: %s", err->message);
    g_error_free (err);
    return;
  }

  g_signal_connect (self->priv->cm, "g-signal",
                    G_CALLBACK (_connman_signal_cb), self);

  g_dbus_proxy_call (self->priv->cm,
                     "GetProperties",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback)_connman_get_properties_cb,
                     self);
}

static void
_connman_appeared_cb (GDBusConnection       *connection,
                      const char            *name,
                      const char            *owner,
                      CarrickConnmanManager *self)
{
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL,
                            "net.connman",
                            "/",
                            "net.connman.Manager",
                            NULL,
                            (GAsyncReadyCallback)_connman_proxy_new_for_bus_cb,
                            self);
}

static void
_connman_vanished_cb (GDBusConnection       *connection,
                      const char            *name,
                      CarrickConnmanManager *self)
{
  if (self->priv->cm) {
    g_signal_handlers_disconnect_by_func (self->priv->cm,
                                          G_CALLBACK (_connman_signal_cb),
                                          self);
    g_object_unref (self->priv->cm);
    self->priv->cm = NULL;
  }

  carrick_connman_manager_set_available_techs (self, NULL);
  carrick_connman_manager_set_enabled_techs (self, NULL);
}

static void
_connman_set_tech_state_cb (GDBusProxy            *cm,
                            GAsyncResult          *res,
                            CarrickConnmanManager *self)
{
  GError *err = NULL;

  g_dbus_proxy_call_finish (cm, res, &err);
  if (err) {
    g_warning ("Failed to change technology state using Connman: %s",
               err->message);
    g_error_free (err);

    /* fake property change -- useful for UIs as they can now reset the
     * toggle that the user foolishly imagined he could toggle */
    g_object_notify (G_OBJECT (self), "enabled-technologies");
  }
}

static gboolean
_strv_equal (const char **strv1,
             const char **strv2)
{
  if (!strv1 && !strv2)
    return TRUE;
  else if (!strv1 || !strv2)
    return FALSE;

  while (*strv1 || *strv2) {
    if (g_strcmp0 (*strv1, *strv2) != 0)
      return FALSE;
    strv1++;
    strv2++;
  }
  return TRUE;
}

static void
carrick_connman_manager_set_available_techs (CarrickConnmanManager  *self,
                                             const char            **techs)
{
  if (!_strv_equal (techs, (const char**)self->priv->available_techs)) {
    g_strfreev (self->priv->available_techs);
    self->priv->available_techs = g_strdupv ((char**)techs);

    g_object_notify (G_OBJECT (self), "available-technologies");
  }
}

static void
carrick_connman_manager_set_enabled_techs (CarrickConnmanManager  *self,
                                           const char            **techs)
{
  if (!_strv_equal (techs, (const char**)self->priv->enabled_techs)) {
    g_strfreev (self->priv->enabled_techs);
    self->priv->enabled_techs = g_strdupv ((char **)techs);

    g_object_notify (G_OBJECT (self), "enabled-technologies");
  }
}

static void
carrick_connman_manager_handle_property (CarrickConnmanManager *self,
                                         const char            *key,
                                         GVariant              *value)
{
  const char **techs;

  if (g_strcmp0 (key, "AvailableTechnologies") == 0) {
    techs = g_variant_get_strv (value, NULL);
    carrick_connman_manager_set_available_techs (self, techs);
  } else if (g_strcmp0 (key, "EnabledTechnologies") == 0) {
    techs = g_variant_get_strv (value, NULL);
    carrick_connman_manager_set_enabled_techs (self, techs);
  }
}

static void
carrick_connman_manager_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  CarrickConnmanManager *self = CARRICK_CONNMAN_MANAGER (object);

  switch (property_id)
    {
    case PROP_ENABLED_TECHNOLOGIES:
      g_value_set_boxed (value, self->priv->enabled_techs);
      break;
    case PROP_AVAILABLE_TECHNOLOGIES:
      g_value_set_boxed (value, self->priv->available_techs);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_connman_manager_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_connman_manager_dispose (GObject *object)
{
  CarrickConnmanManager *self = CARRICK_CONNMAN_MANAGER (object);

   if (self->priv->cm) {
    g_signal_handlers_disconnect_by_func (self->priv->cm,
                                          G_CALLBACK (_connman_signal_cb),
                                          self);
    g_object_unref (self->priv->cm);
  }

  g_strfreev (self->priv->available_techs);
  self->priv->available_techs = NULL;

  g_strfreev (self->priv->enabled_techs);
  self->priv->enabled_techs = NULL;

  G_OBJECT_CLASS (carrick_connman_manager_parent_class)->dispose (object);
}

static void
carrick_connman_manager_class_init (CarrickConnmanManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (CarrickConnmanManagerPrivate));

  object_class->get_property = carrick_connman_manager_get_property;
  object_class->set_property = carrick_connman_manager_set_property;
  object_class->dispose = carrick_connman_manager_dispose;

  pspec = g_param_spec_boxed ("enabled-technologies",
                              "Enabled technologies",
                              "Currently enabled Connman technologies",
                              G_TYPE_STRV,
                              G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_ENABLED_TECHNOLOGIES,
                                   pspec);

  pspec = g_param_spec_boxed ("available-technologies",
                              "Available technologies",
                              "Currently Available Connman technologies",
                              G_TYPE_STRV,
                              G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_AVAILABLE_TECHNOLOGIES,
                                   pspec);
}

static void
carrick_connman_manager_init (CarrickConnmanManager *self)
{
  self->priv = CONNMAN_MANAGER_PRIVATE (self);

  g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                    "net.connman",
                    G_BUS_NAME_WATCHER_FLAGS_NONE,
                    (GBusNameAppearedCallback)_connman_appeared_cb,
                    (GBusNameVanishedCallback)_connman_vanished_cb,
                    self, NULL);
}

void
carrick_connman_manager_set_technology_state (CarrickConnmanManager *self,
                                              const char            *tech,
                                              gboolean               enabled)
{
  g_dbus_proxy_call (self->priv->cm,
                     enabled ? "EnableTechnology" : "DisableTechnology",
                     g_variant_new ("(s)", tech),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback)_connman_set_tech_state_cb,
                     self);
}

CarrickConnmanManager *
carrick_connman_manager_new (void)
{
  return g_object_new (CARRICK_TYPE_CONNMAN_MANAGER, NULL);
}

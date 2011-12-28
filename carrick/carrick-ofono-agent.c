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

#include <gio/gio.h>
#include <string.h>

#include "carrick-ofono-agent.h"
#include "carrick-marshal.h"

#define OFONO_SERVICE           "org.ofono"
#define OFONO_MANAGER_PATH      "/"
#define OFONO_MANAGER_INTERFACE OFONO_SERVICE ".Manager"
#define OFONO_MODEM_INTERFACE   OFONO_SERVICE ".Modem"
#define OFONO_SIM_INTERFACE     OFONO_SERVICE ".SimManager"

G_DEFINE_TYPE (CarrickOfonoAgent, carrick_ofono_agent, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_OFONO_AGENT, CarrickOfonoAgentPrivate))

typedef struct Modem {
  GDBusProxy *modem;
  GDBusProxy *sim;

  gboolean present;
  char *required_pin;
  GHashTable *retries;
} Modem;

struct _CarrickOfonoAgentPrivate {
  GDBusProxy *ofono_mgr;
  GHashTable *modems;

  guint present_sims;
  GHashTable *required_pins;
};

enum {
  PROP_0,
  PROP_N_PRESENT_SIMS,
  PROP_REQUIRED_PINS
};


/* ---- Following types/functions adapted from ofono ---- */
enum ofono_sim_password_type {
	OFONO_SIM_PASSWORD_NONE = 0,
	OFONO_SIM_PASSWORD_SIM_PIN,
	OFONO_SIM_PASSWORD_PHSIM_PIN,
	OFONO_SIM_PASSWORD_PHFSIM_PIN,
	OFONO_SIM_PASSWORD_SIM_PIN2,
	OFONO_SIM_PASSWORD_PHNET_PIN,
	OFONO_SIM_PASSWORD_PHNETSUB_PIN,
	OFONO_SIM_PASSWORD_PHSP_PIN,
	OFONO_SIM_PASSWORD_PHCORP_PIN,
	OFONO_SIM_PASSWORD_SIM_PUK,
	OFONO_SIM_PASSWORD_PHFSIM_PUK,
	OFONO_SIM_PASSWORD_SIM_PUK2,
	OFONO_SIM_PASSWORD_PHNET_PUK,
	OFONO_SIM_PASSWORD_PHNETSUB_PUK,
	OFONO_SIM_PASSWORD_PHSP_PUK,
	OFONO_SIM_PASSWORD_PHCORP_PUK,
	OFONO_SIM_PASSWORD_INVALID,
};

static const char *const passwd_name[] = {
	[OFONO_SIM_PASSWORD_NONE] = "none",
	[OFONO_SIM_PASSWORD_SIM_PIN] = "pin",
	[OFONO_SIM_PASSWORD_SIM_PUK] = "puk",
	[OFONO_SIM_PASSWORD_PHSIM_PIN] = "phone",
	[OFONO_SIM_PASSWORD_PHFSIM_PIN] = "firstphone",
	[OFONO_SIM_PASSWORD_PHFSIM_PUK] = "firstphonepuk",
	[OFONO_SIM_PASSWORD_SIM_PIN2] = "pin2",
	[OFONO_SIM_PASSWORD_SIM_PUK2] = "puk2",
	[OFONO_SIM_PASSWORD_PHNET_PIN] = "network",
	[OFONO_SIM_PASSWORD_PHNET_PUK] = "networkpuk",
	[OFONO_SIM_PASSWORD_PHNETSUB_PIN] = "netsub",
	[OFONO_SIM_PASSWORD_PHNETSUB_PUK] = "netsubpuk",
	[OFONO_SIM_PASSWORD_PHSP_PIN] = "service",
	[OFONO_SIM_PASSWORD_PHSP_PUK] = "servicepuk",
	[OFONO_SIM_PASSWORD_PHCORP_PIN] = "corp",
	[OFONO_SIM_PASSWORD_PHCORP_PUK] = "corppuk",
};

static enum ofono_sim_password_type sim_string_to_passwd(const char *name)
{
	int len = sizeof(passwd_name) / sizeof(*passwd_name);
	int i;

	for (i = 0; i < len; i++)
		if (!g_strcmp0(passwd_name[i], name))
			return i;

	return OFONO_SIM_PASSWORD_INVALID;
}

static gboolean password_is_pin(enum ofono_sim_password_type type)
{
	switch (type) {
	case OFONO_SIM_PASSWORD_SIM_PIN:
	case OFONO_SIM_PASSWORD_PHSIM_PIN:
	case OFONO_SIM_PASSWORD_PHFSIM_PIN:
	case OFONO_SIM_PASSWORD_SIM_PIN2:
	case OFONO_SIM_PASSWORD_PHNET_PIN:
	case OFONO_SIM_PASSWORD_PHNETSUB_PIN:
	case OFONO_SIM_PASSWORD_PHSP_PIN:
	case OFONO_SIM_PASSWORD_PHCORP_PIN:
		return TRUE;
  default:
		return FALSE;
	}
}

static gboolean password_is_puk(enum ofono_sim_password_type type)
{
	switch (type) {
	case OFONO_SIM_PASSWORD_SIM_PUK:
	case OFONO_SIM_PASSWORD_PHFSIM_PUK:
	case OFONO_SIM_PASSWORD_SIM_PUK2:
	case OFONO_SIM_PASSWORD_PHNET_PUK:
	case OFONO_SIM_PASSWORD_PHNETSUB_PUK:
	case OFONO_SIM_PASSWORD_PHSP_PUK:
	case OFONO_SIM_PASSWORD_PHCORP_PUK:
		return TRUE;
  default:
		return FALSE;
	}
}

static enum ofono_sim_password_type puk2pin(enum ofono_sim_password_type type)
{
	switch (type) {
	case OFONO_SIM_PASSWORD_SIM_PUK:
		return OFONO_SIM_PASSWORD_SIM_PIN;
	case OFONO_SIM_PASSWORD_PHFSIM_PUK:
		return OFONO_SIM_PASSWORD_PHFSIM_PIN;
	case OFONO_SIM_PASSWORD_SIM_PUK2:
		return OFONO_SIM_PASSWORD_SIM_PIN2;
	case OFONO_SIM_PASSWORD_PHNET_PUK:
		return OFONO_SIM_PASSWORD_PHNET_PUK;
	case OFONO_SIM_PASSWORD_PHNETSUB_PUK:
		return OFONO_SIM_PASSWORD_PHNETSUB_PIN;
	case OFONO_SIM_PASSWORD_PHSP_PUK:
		return OFONO_SIM_PASSWORD_PHSP_PIN;
	case OFONO_SIM_PASSWORD_PHCORP_PUK:
		return OFONO_SIM_PASSWORD_PHCORP_PIN;
	default:
		return OFONO_SIM_PASSWORD_INVALID;
	}
}

static gboolean
is_valid_pin(const char *pin, unsigned int min,
             unsigned int max)
{
	unsigned int i;

	/* Pin must not be empty */
	if (pin == NULL || pin[0] == '\0')
		return FALSE;

	i = strlen(pin);
	if (i != strspn(pin, "0123456789"))
		return FALSE;

	if (min <= i && i <= max)
		return TRUE;

	return FALSE;
}

/* ----- */


static void
carrick_ofono_agent_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CarrickOfonoAgent *self = CARRICK_OFONO_AGENT (object);

  switch (property_id) {
  case PROP_N_PRESENT_SIMS:
    g_value_set_uint (value, self->priv->present_sims);
    break;
  case PROP_REQUIRED_PINS:
    g_value_set_boxed (value, self->priv->required_pins);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_ofono_agent_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_ofono_agent_class_init (CarrickOfonoAgentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (CarrickOfonoAgentPrivate));

  object_class->get_property = carrick_ofono_agent_get_property;
  object_class->set_property = carrick_ofono_agent_set_property;

  pspec = g_param_spec_uint ("n-present-sims",
                             "n-present-sims",
                             "Number of modems with a SIM",
                             0, G_MAXUINT, 0,
                             G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_N_PRESENT_SIMS,
                                   pspec);

  pspec = g_param_spec_boxed ("required-pins",
                              "required-pins",
                              "List of object paths for modems and the pin types they require",
                              G_TYPE_HASH_TABLE,
                             G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_REQUIRED_PINS,
                                   pspec);

}

static void
carrick_ofono_agent_sim_property_changed (CarrickOfonoAgent *self,
                                          Modem *modem,
                                          const char *key,
                                          GVariant *value)
{
  if (g_strcmp0 (key, "Present") == 0) {
    if (modem->present != g_variant_get_boolean (value)) {
      modem->present = !modem->present;
      if (modem->present)
        self->priv->present_sims++;
      else
        self->priv->present_sims--;
      g_object_notify (G_OBJECT (self), "n-present-sims");
    }

  } else if (g_strcmp0 (key, "PinRequired") == 0) {
    const char *type;

    type = g_variant_get_string (value, NULL);
    if (g_strcmp0 (type, "none") == 0)
      type = NULL;

    if (g_strcmp0 (type, modem->required_pin) != 0) {
      const char *path;

      if (modem->required_pin)
        g_free (modem->required_pin);
      modem->required_pin = g_strdup (type);

      path = g_dbus_proxy_get_object_path (modem->modem);
      if (modem->required_pin)
        g_hash_table_insert (self->priv->required_pins,
                             (char*)path, modem->required_pin);
      else
        g_hash_table_remove (self->priv->required_pins, path);      

      g_object_notify (G_OBJECT (self), "required-pins");
    }

  } else if (g_strcmp0 (key, "Retries") == 0) {
    GVariantIter *iter;
    const char *type;
    guchar retries;

    g_variant_get (value, "a{sy}", &iter);

    g_hash_table_remove_all (modem->retries);
    while (g_variant_iter_loop (iter, "{sy}", &type, &retries)) {
      g_hash_table_insert (modem->retries,
                           g_strdup (type),
                           GINT_TO_POINTER((int)retries));
    }
    g_variant_iter_free (iter);
  }   
}

static void
carrick_ofono_agent_sim_signal_cb (GDBusProxy *sim, 
                                   gchar      *sender_name,
                                   gchar      *signal_name,
                                   GVariant   *parameters,
                                   CarrickOfonoAgent *self)
{
  GVariant *value;
  const char *key;

  if (g_strcmp0 (signal_name, "PropertyChanged") == 0) {
    Modem *modem;

    modem = g_hash_table_lookup (self->priv->modems,
                                 g_dbus_proxy_get_object_path (sim));
    if (modem) {
      g_variant_get (parameters, "(sv)", &key, &value);
      carrick_ofono_agent_sim_property_changed (self, modem, key, value);
    }
  }
}

static void
_get_sim_properties_cb (GDBusProxy *sim,
                        GAsyncResult *res,
                        CarrickOfonoAgent *self)
{
  GError *err = NULL;
  GVariant *props, *value;
  GVariantIter *iter;
  const char *key;
  Modem *modem;

  modem = g_hash_table_lookup (self->priv->modems,
                               g_dbus_proxy_get_object_path (sim));
  if (!modem) {    
    g_warning ("No modem found for %s", g_dbus_proxy_get_object_path (sim));
    g_object_unref (sim);
    return;
  }

  props = g_dbus_proxy_call_finish (sim, res, &err);
  if (err) {
    g_warning ("GetProperties failed: %s", err->message);
    g_error_free (err);
    return;
  }

  g_variant_get (props, "(a{sv})", &iter);
  while (g_variant_iter_loop (iter, "{sv}", &key, &value)) {
    carrick_ofono_agent_sim_property_changed (self, modem, key, value);
  }

  g_variant_iter_free (iter);
  g_variant_unref (props);
}

static void
_sim_proxy_new_cb (GObject *source_object,
                   GAsyncResult *res,
                   CarrickOfonoAgent *self)
{
  GError *err = NULL;
  GDBusProxy *sim;
  Modem *modem;

  sim = g_dbus_proxy_new_for_bus_finish (res, &err);
  if (err) {    
    g_warning ("No SimManager proxy: %s", err->message);
    g_error_free (err);
    return;
  }

  modem = g_hash_table_lookup (self->priv->modems,
                               g_dbus_proxy_get_object_path (sim));
  if (!modem) {    
    g_warning ("No modem found for sim");
    g_object_unref (sim);
    return;
  }

  modem->sim = sim;
  g_signal_connect (modem->sim, "g-signal",
                    G_CALLBACK (carrick_ofono_agent_sim_signal_cb), self);
  g_dbus_proxy_call (modem->sim,
                     "GetProperties",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback)_get_sim_properties_cb,
                     self);
}

static void
carrick_ofono_agent_add_sim_to_modem (CarrickOfonoAgent *self, Modem *modem)
{
  if (modem->sim)
    return;

  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL, /* should add iface info here */
                            OFONO_SERVICE,
                            g_dbus_proxy_get_object_path (modem->modem),
                            OFONO_SIM_INTERFACE,
                            NULL,
                            (GAsyncReadyCallback)_sim_proxy_new_cb,
                            self);
}

static void
carrick_ofono_agent_remove_sim_from_modem (CarrickOfonoAgent *self, Modem *modem)
{
  if (!modem->sim)
    return;

  g_object_unref (modem->sim);
  modem->sim = NULL;

  if (modem->present) {
    self->priv->present_sims--;
    g_object_notify (G_OBJECT (self), "n-present-sims");
  }
  modem->present = FALSE;

  /* check if the modem was requiring a pin */
  if (g_hash_table_remove (self->priv->required_pins,
                           g_dbus_proxy_get_object_path (modem->modem))) {
    g_object_notify (G_OBJECT (self), "required-pins");
  }
}

static void
carrick_ofono_agent_modem_property_changed (CarrickOfonoAgent *self,
                                            Modem *modem,
                                            const char *key,
                                            GVariant *value)
{
  if (g_strcmp0 (key, "Interfaces") == 0) {
    gboolean has_sim_manager = FALSE;
    const char **ifaces, **iter;

    ifaces = iter = g_variant_get_strv (value, NULL);
    while (*iter) {
      if (g_strcmp0 (*iter, "org.ofono.SimManager") == 0) {
        has_sim_manager = TRUE;
        break;
      }
      iter++;
    }
    g_free (ifaces);

    if (has_sim_manager) {
      carrick_ofono_agent_add_sim_to_modem (self, modem);
    } else {
      carrick_ofono_agent_remove_sim_from_modem (self, modem);
    }
  }
}

static void
carrick_ofono_agent_modem_signal_cb (GDBusProxy *proxy,
                                     gchar      *sender_name,
                                     gchar      *signal_name,
                                     GVariant   *parameters,
                                     CarrickOfonoAgent *self)
{
  GVariant *value;
  const char *key;

  if (g_strcmp0 (signal_name, "PropertyChanged") == 0) {
    Modem *modem;

    modem = g_hash_table_lookup (self->priv->modems,
                                 g_dbus_proxy_get_object_path (proxy));
    if (modem) {
      g_variant_get (parameters, "(sv)", &key, &value);
      carrick_ofono_agent_modem_property_changed (self, modem, key, value);
    }
  }
}

static void
modem_free (Modem *modem)
{
  if (modem->modem)
    g_object_unref (modem->modem);
  if (modem->sim)
    g_object_unref (modem->sim);
  if (modem->retries)
    g_hash_table_destroy (modem->retries);
  if (modem->required_pin)
    g_free (modem->required_pin);
  g_slice_free (Modem, modem);
}

static void
_get_modem_properties_cb (GDBusProxy *modem_proxy,
                          GAsyncResult *res,
                          CarrickOfonoAgent *self)
{
  GError *err = NULL;
  GVariant *props, *value;
  GVariantIter *iter;
  const char *key;
  Modem *modem;

  modem = g_hash_table_lookup (self->priv->modems,
                               g_dbus_proxy_get_object_path (modem_proxy));
  if (!modem) {    
    g_warning ("No modem found for %s", g_dbus_proxy_get_object_path (modem_proxy));
    g_object_unref (modem_proxy);
    return;
  }

  props = g_dbus_proxy_call_finish (modem_proxy, res, &err);
  if (err) {
    g_warning ("GetProperties failed: %s", err->message);
    g_error_free (err);
    return;
  }

  g_variant_get (props, "(a{sv})", &iter);
  while (g_variant_iter_loop (iter, "{sv}", &key, &value)) {
    carrick_ofono_agent_modem_property_changed (self, modem, key, value);
  }

  g_variant_iter_free (iter);
  g_variant_unref (props);
}

static void
_modem_proxy_new_cb (GObject *source_object,
                     GAsyncResult *res,
                     CarrickOfonoAgent *self)
{
  GError *err = NULL;
  GDBusProxy *modem_proxy;
  Modem *modem;


  modem_proxy = g_dbus_proxy_new_for_bus_finish (res, &err);
  if (err) {
    g_warning ("No Modem proxy: %s", err->message);
    g_error_free (err);
    return;
  }

  modem = g_slice_new0 (Modem);
  modem->retries = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          g_free, NULL);
  modem->modem = modem_proxy;
  g_hash_table_insert (self->priv->modems,
                       g_strdup (g_dbus_proxy_get_object_path (modem->modem)),
                       modem);

  g_signal_connect (modem->modem, "g-signal",
                    G_CALLBACK (carrick_ofono_agent_modem_signal_cb), self);
  g_dbus_proxy_call (modem->modem,
                     "GetProperties",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback)_get_modem_properties_cb,
                     self);
}

static void
carrick_ofono_agent_add_modem (CarrickOfonoAgent *self,
                               const char *obj_path)
{
  /* We need Modem interface only for seeing when the object starts/stops 
   * supporting SimManager */
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL, /* should add iface info here */
                            OFONO_SERVICE,
                            obj_path,
                            OFONO_MODEM_INTERFACE,
                            NULL,
                            (GAsyncReadyCallback)_modem_proxy_new_cb,
                            self);
}

static void
carrick_ofono_agent_remove_modem (CarrickOfonoAgent *self,
                                  const char *obj_path)
{
  Modem *modem;

  modem = g_hash_table_lookup (self->priv->modems, obj_path);
  if (modem) {
    carrick_ofono_agent_remove_sim_from_modem (self, modem);
    g_hash_table_remove (self->priv->modems, obj_path);
  }
}

static void
carrick_ofono_manager_signal_cb (GDBusProxy *ofono_mgr, 
                                 gchar      *sender_name,
                                 gchar      *signal_name,
                                 GVariant   *parameters,
                                 CarrickOfonoAgent *self)
{
  const char *obj_path;

  if (g_strcmp0 (signal_name, "ModemAdded") == 0) {
    g_variant_get (parameters, "(oa{sv})", &obj_path, NULL);
    carrick_ofono_agent_add_modem (self, obj_path);
  } else if (g_strcmp0 (signal_name, "ModemRemoved") == 0) {
    g_variant_get (parameters, "(o)", &obj_path);
    carrick_ofono_agent_remove_modem (self, obj_path);
  }
}

static void
_get_modems_cb (GDBusProxy *ofono_mgr,
                GAsyncResult *res,
                CarrickOfonoAgent *self)
{
  GVariant *modems;
  char *obj_path;
  GVariantIter *iter;
  GError *err = NULL;

  modems = g_dbus_proxy_call_finish (ofono_mgr, res, &err);
  if (err) {
    g_warning ("GetModems failed: %s", err->message);
    return;
  }

  g_variant_get (modems, "(a(oa{sv}))", &iter);
  while (g_variant_iter_loop (iter, "(oa{sv})", &obj_path, NULL)) {
    carrick_ofono_agent_add_modem (self, obj_path);
  }
  g_variant_iter_free (iter);
  g_variant_unref (modems);
}

static void
ofono_proxy_new_for_bus_cb (GObject *object,
                            GAsyncResult *res,
                            CarrickOfonoAgent *self)
{
  GError *err = NULL;

  self->priv->ofono_mgr = g_dbus_proxy_new_for_bus_finish (res, &err);
  if (err) {
    g_warning ("No ofono proxy: %s", err->message);
    return;
  }

  g_signal_connect (self->priv->ofono_mgr, "g-signal",
                    G_CALLBACK (carrick_ofono_manager_signal_cb), self);

  g_dbus_proxy_call (self->priv->ofono_mgr,
                     "GetModems",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback)_get_modems_cb,
                     self);
}

static void
carrick_ofono_agent_init (CarrickOfonoAgent *self)
{
  self->priv = GET_PRIVATE (self);

  self->priv->modems = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              g_free, (GDestroyNotify)modem_free);
  self->priv->required_pins = g_hash_table_new (g_str_hash, g_str_equal);

  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL, /* should add iface info here */
                            OFONO_SERVICE,
                            OFONO_MANAGER_PATH,
                            OFONO_MANAGER_INTERFACE,
                            NULL,
                            (GAsyncReadyCallback)ofono_proxy_new_for_bus_cb,
                            self);
}

CarrickOfonoAgent*
carrick_ofono_agent_new (void)
{
  return g_object_new (CARRICK_TYPE_OFONO_AGENT, NULL);
}

typedef struct CarrickOfonoData{
  CarrickOfonoAgent *agent;
  const char *modem;
  CarrickOfonoAgentCallback callback;
  gpointer userdata;
} CarrickOfonoData;

CarrickOfonoData*
carrick_ofono_data_new (CarrickOfonoAgent *agent,
                        Modem *modem,
                        CarrickOfonoAgentCallback callback,
                        gpointer userdata)
{
  CarrickOfonoData *data;
  data = g_slice_new (CarrickOfonoData);
  data->agent = agent;
  data->modem = g_dbus_proxy_get_object_path (modem->modem); /* copy? */
  data->callback = callback;
  data->userdata = userdata;

  return data;
}

static void 
_carrick_ofono_agent_generic_cb (GObject *source_object,
                                 GAsyncResult *res,
                                 CarrickOfonoData *data)
{
  data->callback (data->agent, data->modem, res, data->userdata);
  g_slice_free (CarrickOfonoData, data);
}

gboolean
carrick_ofono_agent_finish (CarrickOfonoAgent  *self,
                            const char         *modem_path,
                            GAsyncResult       *res,
                            GError            **error)
{
  Modem *modem;

  modem = g_hash_table_lookup (self->priv->modems, modem_path);
  if (!modem) {
    /* TODO g_simple_async_result_new_error ... */
    return FALSE;
  }

  g_variant_unref (g_dbus_proxy_call_finish (modem->sim, res, error));
  return (error == NULL);
}

void
carrick_ofono_agent_enter_pin (CarrickOfonoAgent   *self,
                               const char          *modem_path,
                               const char          *pin_type,
                               const char          *pin,
                               CarrickOfonoAgentCallback callback,
                               gpointer             user_data)
{
  enum ofono_sim_password_type pw_type;
  Modem *modem;

  modem = g_hash_table_lookup (self->priv->modems, modem_path);
  pw_type = sim_string_to_passwd (pin_type);

  if (!self || !modem_path || !pin_type || !pin || !modem || !password_is_pin (pw_type))
    g_warning ("EnterPin arguments seem wrong");
  if (!carrick_ofono_is_valid_pin (pin, pin_type))
    g_warning ("EnterPin pin seems invalid");

  g_dbus_proxy_call (modem->sim,
                     "EnterPin",
                     g_variant_new ("(ss)", pin_type, pin),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback)_carrick_ofono_agent_generic_cb,
                     carrick_ofono_data_new (self, modem, callback, user_data));
}

void
carrick_ofono_agent_reset_pin (CarrickOfonoAgent   *self,
                               const char          *modem_path,
                               const char          *puk_type,
                               const char          *puk,
                               const char          *new_pin,
                               CarrickOfonoAgentCallback callback,
                               gpointer             user_data)
{
  enum ofono_sim_password_type pw_type;
  Modem *modem;

  pw_type = sim_string_to_passwd (puk_type);
  modem = g_hash_table_lookup (self->priv->modems, modem_path);

  if (!modem || !password_is_puk (pw_type))
    g_warning ("ResetPin arguments seem wrong");

  g_dbus_proxy_call (modem->sim,
                     "ResetPin",
                     g_variant_new ("(sss)", puk_type, puk, new_pin),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback)_carrick_ofono_agent_generic_cb,
                     carrick_ofono_data_new (self, modem, callback, user_data));
}

void
carrick_ofono_agent_change_pin (CarrickOfonoAgent *self,
                                const char        *modem_path,
                                const char        *pin_type,
                                const char        *pin,
                                const char        *new_pin,
                               CarrickOfonoAgentCallback callback,
                                gpointer             user_data)
{
  enum ofono_sim_password_type pw_type;
  Modem *modem;

  pw_type = sim_string_to_passwd (pin_type);
  modem = g_hash_table_lookup (self->priv->modems, modem_path);

  if (!modem || !password_is_pin (pw_type))
    g_warning ("ChangePin arguments seem wrong");
  if (!carrick_ofono_is_valid_pin (pin, pin_type) ||
      !carrick_ofono_is_valid_pin (new_pin, pin_type))
    g_warning ("ChangePin pin seems invalid");

  g_dbus_proxy_call (modem->sim,
                     "ChangePin",
                     g_variant_new ("(sss)", pin_type, pin, new_pin),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                    (GAsyncReadyCallback)_carrick_ofono_agent_generic_cb,
                    carrick_ofono_data_new (self, modem, callback, user_data));
}

int
carrick_ofono_agent_get_retries (CarrickOfonoAgent *self,
                                 const char        *modem_path,
                                 const char        *pin_type)
{
  Modem *modem;
  gpointer p;

  modem = g_hash_table_lookup (self->priv->modems, modem_path);

  g_return_val_if_fail (modem, -1);

  p = g_hash_table_lookup (modem->retries, pin_type);
  if (!p)
    return -1;
  return GPOINTER_TO_INT (p);
}

gboolean
carrick_ofono_is_pin (const char *pin_type)
{
  return password_is_pin (sim_string_to_passwd (pin_type));
}

gboolean
carrick_ofono_is_puk (const char *pin_type)
{
  return password_is_puk (sim_string_to_passwd (pin_type));
}


const char*
carrick_ofono_pin_for_puk (const char *puk_type)
{
  enum ofono_sim_password_type pin_type;

  pin_type = puk2pin (sim_string_to_passwd (puk_type));
  return passwd_name[pin_type];
}

gboolean
carrick_ofono_is_valid_pin(const char *pin,
                           const char *pin_type)
{
  enum ofono_sim_password_type type;

  type = sim_string_to_passwd (pin_type);
	switch (type) {
	case OFONO_SIM_PASSWORD_SIM_PIN:
	case OFONO_SIM_PASSWORD_SIM_PIN2:
		/* 11.11 Section 9.3 ("CHV"): 4..8 IA-5 digits */
		return is_valid_pin(pin, 4, 8);
		break;
	case OFONO_SIM_PASSWORD_PHSIM_PIN:
	case OFONO_SIM_PASSWORD_PHFSIM_PIN:
	case OFONO_SIM_PASSWORD_PHNET_PIN:
	case OFONO_SIM_PASSWORD_PHNETSUB_PIN:
	case OFONO_SIM_PASSWORD_PHSP_PIN:
	case OFONO_SIM_PASSWORD_PHCORP_PIN:
		/* 22.022 Section 14 4..16 IA-5 digits */
		return is_valid_pin(pin, 4, 16);
		break;
	case OFONO_SIM_PASSWORD_SIM_PUK:
	case OFONO_SIM_PASSWORD_SIM_PUK2:
	case OFONO_SIM_PASSWORD_PHFSIM_PUK:
	case OFONO_SIM_PASSWORD_PHNET_PUK:
	case OFONO_SIM_PASSWORD_PHNETSUB_PUK:
	case OFONO_SIM_PASSWORD_PHSP_PUK:
	case OFONO_SIM_PASSWORD_PHCORP_PUK:
		/* 11.11 Section 9.3 ("UNBLOCK CHV"), 8 IA-5 digits */
		return is_valid_pin(pin, 8, 8);
		break;
	case OFONO_SIM_PASSWORD_NONE:
		return is_valid_pin(pin, 0, 8);
		break;
	case OFONO_SIM_PASSWORD_INVALID:
		break;
	}

	return FALSE;
}

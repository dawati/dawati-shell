/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Author: Jussi Kukkonen <jussi.kukkonen@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <glib/gi18n.h>

#include "dawati-bt-request.h"
#include "dawati-bt-marshal.h"

/* TODO: fill this list with translatable names for BT services */
/* TRANSLATORS: these are names of Bluetooth services. They will be used in 
 * messages like "%s wants access to %s service" */
static const char *uuids[][2] = {
  {"0000111f-0000-1000-8000-00805f9b34fb", N_("Hands-free Audio Gateway")},
  {NULL, NULL}
};
G_DEFINE_TYPE (DawatiBtRequest, dawati_bt_request, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DAWATI_TYPE_BT_REQUEST, DawatiBtRequestPrivate))

typedef struct _DawatiBtRequestPrivate DawatiBtRequestPrivate;

struct _DawatiBtRequestPrivate {
  ClutterActor *title;
  ClutterActor *request_label;
  ClutterActor *request_entry;
  ClutterActor *request_always_btn;
  ClutterActor *request_yes_btn;
  ClutterActor *request_no_btn;

  char *device_path;
  DawatiBtRequestType request;
  char *request_data;

  GHashTable *uuid_strings;
};

enum {
  SIGNAL_RESPONSE,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

enum
{
  PROP_0,
  PROP_NAME,
  PROP_DEVICE_PATH,
  PROP_REQUEST_TYPE,
  PROP_REQUEST_DATA,
};

static void
dawati_bt_request_update_strings (DawatiBtRequest *request)
{
  DawatiBtRequestPrivate *priv = GET_PRIVATE (request);
  const char *name = NULL;
  char *msg = NULL;
  const char *no = "";
  const char *yes = "";

  switch (priv->request) {
    case DAWATI_BT_REQUEST_TYPE_AUTH:
      if (priv->request_data) {
        name = g_hash_table_lookup (priv->uuid_strings, priv->request_data);
        if (name) {
          msg = g_strdup_printf (_("wants access to %s service."), name);
        } else {
          g_debug ("Unrecognised service '%s' requested", priv->request_data);
          msg = g_strdup (_("wants access to a service."));
        }
        yes = _("Grant this time only");
        no = _("Reject");
      }
      break;
    case DAWATI_BT_REQUEST_TYPE_PIN:
    case DAWATI_BT_REQUEST_TYPE_PASSKEY:
      msg = g_strdup (_("wants to pair with this computer. "
                        "Please enter the PIN mentioned on the device."));
      yes = _("OK");
      no = _("Cancel");
      break;
    case DAWATI_BT_REQUEST_TYPE_CONFIRM:
      msg = g_strdup_printf (_("wants to pair with this computer. "
                               "Please confirm whether the PIN '%s' matches the one on the device."),
                             priv->request_data);
      yes = _("Matches");
      no = _("Does not match");
      break;
    default:
      break;
  }

  mx_label_set_text (MX_LABEL (priv->request_label), msg);
  mx_button_set_label (MX_BUTTON (priv->request_no_btn), no);
  mx_button_set_label (MX_BUTTON (priv->request_yes_btn), yes);

  g_free (msg);
}

static void
dawati_bt_request_set_request_type (DawatiBtRequest *request,
                                   DawatiBtRequestType type)
{
  DawatiBtRequestPrivate *priv = GET_PRIVATE (request);

  priv->request = type;
  switch (type) {
    case DAWATI_BT_REQUEST_TYPE_AUTH:
      clutter_actor_hide (priv->request_entry);
      clutter_actor_show (priv->request_always_btn);
      break;
    case DAWATI_BT_REQUEST_TYPE_PIN:
    case DAWATI_BT_REQUEST_TYPE_PASSKEY:
      clutter_actor_show (priv->request_entry);
      clutter_actor_hide (priv->request_always_btn);
      break;
    case DAWATI_BT_REQUEST_TYPE_CONFIRM:
      clutter_actor_hide (priv->request_entry);
      clutter_actor_hide (priv->request_always_btn);
      break;
    default:
      ;
  }

  dawati_bt_request_update_strings (request);
}

static void
dawati_bt_request_set_request_data (DawatiBtRequest *request,
                                   const char *data)
{
  DawatiBtRequestPrivate *priv = GET_PRIVATE (request);

  g_free (priv->request_data);
  priv->request_data = g_strdup (data);

  dawati_bt_request_update_strings (request);
}

static void
dawati_bt_request_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  DawatiBtRequestPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_NAME:
      g_value_set_string (value, mx_label_get_text (MX_LABEL (priv->title)));
      break;
    case PROP_DEVICE_PATH:
      g_value_set_string (value, priv->device_path);
      break;
    case PROP_REQUEST_TYPE:
      g_value_set_uint (value, priv->request);
      break;
    case PROP_REQUEST_DATA:
      g_value_set_string (value, priv->request_data);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dawati_bt_request_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  DawatiBtRequestPrivate *priv = GET_PRIVATE (object);
  switch (property_id) {
    case PROP_NAME:
      mx_label_set_text (MX_LABEL (priv->title), g_value_get_string (value));
      break;
    case PROP_DEVICE_PATH:
      g_free (priv->device_path);
      priv->device_path = g_value_dup_string (value);
      break;
    case PROP_REQUEST_TYPE:
      dawati_bt_request_set_request_type (DAWATI_BT_REQUEST (object),
                                          g_value_get_uint (value));
      break;
    case PROP_REQUEST_DATA:
      dawati_bt_request_set_request_data (DAWATI_BT_REQUEST (object),
                                         g_value_get_string (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dawati_bt_request_dispose (GObject *object)
{
  DawatiBtRequestPrivate *priv = GET_PRIVATE (object);

  g_free (priv->device_path);
  priv->device_path = NULL;

  g_free (priv->request_data);
  priv->request_data = NULL;

  if (priv->uuid_strings)
    g_hash_table_destroy (priv->uuid_strings);
  priv->uuid_strings = NULL;

  G_OBJECT_CLASS (dawati_bt_request_parent_class)->dispose (object);
}

static void
dawati_bt_request_class_init (DawatiBtRequestClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (DawatiBtRequestPrivate));

  object_class->get_property = dawati_bt_request_get_property;
  object_class->set_property = dawati_bt_request_set_property;
  object_class->dispose = dawati_bt_request_dispose;

  signals[SIGNAL_RESPONSE] = g_signal_new ("response",
                                           G_TYPE_FROM_CLASS (object_class),
                                           G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
                                           G_STRUCT_OFFSET (DawatiBtRequestClass, response),
                                           NULL, NULL,
                                           dawati_bt_marshal_VOID__UINT_STRING,
                                           G_TYPE_NONE,
                                           2, G_TYPE_UINT, G_TYPE_STRING);

  pspec = g_param_spec_string ("name",
                               "name",
                               "Device name",
                               "",
                               G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_NAME, pspec);

  pspec = g_param_spec_string ("device-path",
                               "device-path",
                               "Bluetooth Device D-Bus path",
                               "",
                               G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DEVICE_PATH, pspec);

  /* _really_ should be an enum */
  pspec = g_param_spec_uint ("request-type",
                             "request-type",
                             "Current request type",
                             DAWATI_BT_REQUEST_TYPE_PIN, DAWATI_BT_REQUEST_TYPE_AUTH,
                             DAWATI_BT_REQUEST_TYPE_PIN,
                             G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_REQUEST_TYPE, pspec);

  pspec = g_param_spec_string ("request-data",
                               "request-data",
                               "Data relating to current request (content depends on request type)",
                               NULL,
                               G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_REQUEST_DATA, pspec);

}

static void
_always_clicked_cb (MxButton *button, DawatiBtRequest *request)
{
  g_signal_emit (request, signals[SIGNAL_RESPONSE], 0,
                 DAWATI_BT_REQUEST_RESPONSE_ALWAYS, NULL);
}

static void
_yes_clicked_cb (MxButton *button, DawatiBtRequest *request)
{
  DawatiBtRequestPrivate *priv = GET_PRIVATE (request);
  const char *response = NULL;

  if (priv->request == DAWATI_BT_REQUEST_TYPE_PIN)
    response = mx_entry_get_text (MX_ENTRY (priv->request_entry));

  g_signal_emit (request, signals[SIGNAL_RESPONSE], 0,
                 DAWATI_BT_REQUEST_RESPONSE_YES, response);
}

static void
_no_clicked_cb (MxButton *button, DawatiBtRequest *request)
{
  g_signal_emit (request, signals[SIGNAL_RESPONSE], 0,
                 DAWATI_BT_REQUEST_RESPONSE_NO, NULL);
}

static GHashTable*
_new_uiid_strings (void)
{
  GHashTable *uuid_strings;
  guint i = 0;

  uuid_strings = g_hash_table_new (g_str_hash, g_str_equal);
  while (uuids[i][0]) {
    g_hash_table_insert (uuid_strings, (char*)uuids[i][0], _(uuids[i][1]));
    i++;
  }
  return uuid_strings;
}

static void
dawati_bt_request_init (DawatiBtRequest *request)
{
  ClutterActor *btn_box;
  DawatiBtRequestPrivate *priv = GET_PRIVATE (request);

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (request),
                                 MX_ORIENTATION_VERTICAL);
  mx_stylable_set_style_class (MX_STYLABLE (request), "contentPanel");

  priv->request = DAWATI_BT_REQUEST_TYPE_PIN;

  /* FIXME: this should maybe not be instance-specific */
  priv->uuid_strings = _new_uiid_strings ();

  priv->title = mx_label_new ();
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (request), priv->title, -1,
                                           "expand", TRUE,
                                           "x-fill", FALSE,
                                           "x-align", MX_ALIGN_START,
                                           NULL);

  priv->request_label = mx_label_new ();
  mx_label_set_line_wrap (MX_LABEL (priv->request_label), TRUE);
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (request), priv->request_label, -1,
                                           "expand", TRUE,
                                           "x-fill", FALSE,
                                           "x-align", MX_ALIGN_START,
                                           NULL);

  btn_box = mx_box_layout_new ();
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (request), btn_box, -1,
                                           "expand", TRUE,
                                           "x-fill", FALSE,
                                           "x-align", MX_ALIGN_END,
                                           NULL);

  priv->request_entry = mx_entry_new ();
  mx_box_layout_add_actor (MX_BOX_LAYOUT (btn_box), priv->request_entry, -1);

  priv->request_always_btn = mx_button_new ();
  mx_button_set_label (MX_BUTTON (priv->request_always_btn),
                       _("Always grant"));
  mx_box_layout_add_actor (MX_BOX_LAYOUT (btn_box), priv->request_always_btn, -1);
  g_signal_connect (priv->request_always_btn, "clicked",
                    G_CALLBACK (_always_clicked_cb), request);

  priv->request_yes_btn = mx_button_new ();
  mx_box_layout_add_actor (MX_BOX_LAYOUT (btn_box), priv->request_yes_btn, -1);
  g_signal_connect (priv->request_yes_btn, "clicked",
                    G_CALLBACK (_yes_clicked_cb), request);

  priv->request_no_btn = mx_button_new ();
  mx_box_layout_add_actor (MX_BOX_LAYOUT (btn_box), priv->request_no_btn, -1);
  g_signal_connect (priv->request_no_btn, "clicked",
                    G_CALLBACK (_no_clicked_cb), request);
}

ClutterActor*
dawati_bt_request_new (const char *name, const char *device_path,
                       DawatiBtRequestType type, const char *data)
{
  return g_object_new (DAWATI_TYPE_BT_REQUEST,
                       "name", name,
                       "device-path", device_path,
                       "request-type", type,
                       "request-data", data,
                       NULL);
}


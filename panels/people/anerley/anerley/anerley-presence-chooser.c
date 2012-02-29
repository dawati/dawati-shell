/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2010, Intel Corporation.
 *
 * Authors: Danielle Madeley <danielle.madeley@collabora.co.uk>
 *          Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#include <config.h>

#include <telepathy-glib/account-manager.h>

#include <glib/gi18n-lib.h>

#include "anerley-presence-chooser.h"

#define GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ANERLEY_TYPE_PRESENCE_CHOOSER, AnerleyPresenceChooserPrivate))

G_DEFINE_TYPE (AnerleyPresenceChooser, anerley_presence_chooser, MX_TYPE_COMBO_BOX);

typedef struct
{
  gchar *name;
  gchar *status;
  gchar *icon;
} PresenceTuple;

static const PresenceTuple presences[NUM_TP_CONNECTION_PRESENCE_TYPES] = {
  { N_("Unset"), NULL, "user-offline"},
  { N_("Offline"), "offline", "user-offline"},
  { N_("Available"), "available", "user-available"},
  { N_("Away"), "away", "user-away"},
  { N_("Extended Away"), "xa", "user-away"},
  { N_("Hidden"), "hidden", "user-offline"},
  { N_("Busy"), "busy", "user-busy"},
  { N_("Unknown"), "unknown", "user-offline"},
  { N_("Error"), "error", "user-offline"},
};

typedef struct
{
  TpConnectionPresenceType presence;
} ComboEntry;

typedef struct _AnerleyPresenceChooserPrivate AnerleyPresenceChooserPrivate;

struct _AnerleyPresenceChooserPrivate
{
  TpAccountManager *am;
  GArray *combo_entries;

  TpConnectionPresenceType presence; /* selected presence */
};

const gchar *
anerley_presence_chooser_get_default_message (TpConnectionPresenceType presence)
{
  const PresenceTuple *p;

  g_return_val_if_fail (presence >= 0 &&
                        presence < NUM_TP_CONNECTION_PRESENCE_TYPES,
                        NULL);

  p = &presences[presence];
  return g_dgettext (GETTEXT_PACKAGE, p->name);
}

gchar *
anerley_presence_chooser_get_icon (TpConnectionPresenceType presence)
{
  const PresenceTuple *p;

  g_return_val_if_fail (presence >= 0 &&
                        presence < NUM_TP_CONNECTION_PRESENCE_TYPES,
                        NULL);

  p = &presences[presence];

  return p->icon;
}

static void
_combo_index_changed (AnerleyPresenceChooser *self,
                      GParamSpec             *pspec,
                      gpointer                user_data)
{
  AnerleyPresenceChooserPrivate *priv = GET_PRIVATE (self);
  MxComboBox *combo = MX_COMBO_BOX (self);
  gint index = mx_combo_box_get_index (combo);
  ComboEntry *entry;
  gchar *message;

  if (index >= 0)
    entry = &g_array_index (priv->combo_entries, ComboEntry, index);
  else
    return;

  priv->presence = entry->presence;

  /* Get current message to not modify it */
  tp_account_manager_get_most_available_presence (priv->am,
                                                  NULL,
                                                  &message);

  tp_account_manager_set_all_requested_presences (priv->am,
                                                  entry->presence,
                                                  presences[entry->presence].status,
                                                  message);

  g_free (message);
}

static void
update_combox_index (AnerleyPresenceChooser *self)
{
  AnerleyPresenceChooserPrivate *priv = GET_PRIVATE (self);
  MxComboBox *combo = MX_COMBO_BOX (self);

  g_signal_handlers_block_by_func (combo, _combo_index_changed, NULL);
  switch (priv->presence)
  {
    case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
      mx_combo_box_set_index (combo, 0);
      break;
    case TP_CONNECTION_PRESENCE_TYPE_BUSY:
      mx_combo_box_set_index (combo, 1);
      break;
    case TP_CONNECTION_PRESENCE_TYPE_AWAY:
    case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
      mx_combo_box_set_index (combo, 2);
      break;
    default:
      mx_combo_box_set_index (combo, 3);
      break;
  }
  g_signal_handlers_unblock_by_func (combo, _combo_index_changed, NULL);
}

static void
_account_manager_presence_changed (TpAccountManager         *am,
                                   TpConnectionPresenceType  presence,
                                   const gchar              *status,
                                   const gchar              *message,
                                   AnerleyPresenceChooser   *self)
{
  AnerleyPresenceChooserPrivate *priv = GET_PRIVATE (self);

  /* only change the presence if it changes */
  if (priv->presence == presence)
    return;

  priv->presence = presence;

  update_combox_index (self);
}

static void
_account_manager_ready (TpAccountManager *am,
                        GAsyncResult     *res,
                        gpointer          user_data)
{
  AnerleyPresenceChooser *self = user_data;
  AnerleyPresenceChooserPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (TP_PROXY (am), res, &error))
  {
    g_warning (G_STRLOC ": Error preparing account manager: %s",
               error->message);
    g_error_free (error);
    return;
  }

  priv->presence = tp_account_manager_get_most_available_presence (priv->am,
                                                                   NULL,
                                                                   NULL);
  update_combox_index (self);
}

static void
anerley_presence_chooser_dispose (GObject *object)
{
  AnerleyPresenceChooser *self = ANERLEY_PRESENCE_CHOOSER (object);
  AnerleyPresenceChooserPrivate *priv = GET_PRIVATE (self);

  if (priv->am != NULL)
    {
      g_object_unref (priv->am);
      priv->am = NULL;
    }

  if (priv->combo_entries != NULL)
    {
      g_array_free (priv->combo_entries, TRUE);
      priv->combo_entries = NULL;
    }
}

static void
anerley_presence_chooser_class_init (AnerleyPresenceChooserClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AnerleyPresenceChooserPrivate));

  gobject_class->dispose = anerley_presence_chooser_dispose;
}

static void
_append_presence (MxComboBox               *combo,
                  TpConnectionPresenceType  presence)
{
  AnerleyPresenceChooserPrivate *priv = GET_PRIVATE (combo);
  ComboEntry entry = { 0, };
  const gchar *name, *icon;

  name = anerley_presence_chooser_get_default_message (presence);
  icon = anerley_presence_chooser_get_icon (presence);

  mx_combo_box_insert_text_with_icon (combo, -1, name, icon);

  entry.presence = presence;
  g_array_append_val (priv->combo_entries, entry);
}

static void
anerley_presence_chooser_init (AnerleyPresenceChooser *self)
{
  MxComboBox *combo = MX_COMBO_BOX (self);
  AnerleyPresenceChooserPrivate *priv = GET_PRIVATE (self);

  mx_stylable_set_style_class (MX_STYLABLE (self), "Primary");

  priv->am = tp_account_manager_dup ();

  g_signal_connect (priv->am,
                    "most-available-presence-changed",
                    G_CALLBACK (_account_manager_presence_changed),
                    self);

  priv->combo_entries = g_array_sized_new (FALSE, TRUE, sizeof (ComboEntry), 7);

  /* add some entries */
  _append_presence (combo, TP_CONNECTION_PRESENCE_TYPE_AVAILABLE);
  _append_presence (combo, TP_CONNECTION_PRESENCE_TYPE_BUSY);
  _append_presence (combo, TP_CONNECTION_PRESENCE_TYPE_AWAY);
  _append_presence (combo, TP_CONNECTION_PRESENCE_TYPE_OFFLINE);
  /* FIXME: Hidden ? */

  g_signal_connect (self,
                    "notify::index",
                    G_CALLBACK (_combo_index_changed),
                    NULL);

  tp_proxy_prepare_async (TP_PROXY (priv->am),
                          NULL,
                          (GAsyncReadyCallback)_account_manager_ready,
                          self);
}

ClutterActor *
anerley_presence_chooser_new (void)
{
  return g_object_new (ANERLEY_TYPE_PRESENCE_CHOOSER, NULL);
}

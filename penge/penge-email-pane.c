/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
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
 */

#include <config.h>
#include <glib/gi18n-lib.h>

#include "penge-email-pane.h"

#include <gtk/gtk.h>

#include <mailme/mailme-telepathy.h>
#include <mailme/mailme-telepathy-account.h>

#include "penge-utils.h"
#include "penge-count-tile.h"

G_DEFINE_TYPE (PengeEmailPane, penge_email_pane, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o),\
                                PENGE_TYPE_EMAIL_PANE, \
                                PengeEmailPanePrivate))


#define GET_PRIVATE(o) ((PengeEmailPane *)o)->priv

struct _PengeEmailPanePrivate {
  MailmeTelepathy *provider;
  GHashTable *account_to_widget;
  gboolean vertical;
};

enum
{
  PROP_0,
  PROP_VERTICAL
};

static void
_widget_foreach_update (MailmeTelepathyAccount *account,
                        ClutterActor           *widget,
                        PengeEmailPane         *pane)
{
   PengeEmailPanePrivate *priv = GET_PRIVATE (pane);
   g_object_set (widget,
                 "compact", priv->vertical,
                 NULL);
}

static void
penge_email_pane_set_vertical (PengeEmailPane *pane,
                               gboolean        vertical)
{
  PengeEmailPanePrivate *priv = GET_PRIVATE (pane);

  if (priv->vertical == vertical)
    return;

  priv->vertical = vertical;
  g_hash_table_foreach (priv->account_to_widget,
                        (GHFunc)_widget_foreach_update,
                        pane);
}

static void
penge_email_pane_dispose (GObject *object)
{
  PengeEmailPanePrivate *priv = GET_PRIVATE (object);
  if (priv->provider)
  {
    g_object_unref (priv->provider);
    priv->provider = NULL;
  }
  if (priv->account_to_widget)
  {
    /* No need to unref the containt since the widgets are owned by the
     * container and the account are own by the provider */
    g_hash_table_unref (priv->account_to_widget);
    priv->account_to_widget = NULL;
  }
  G_OBJECT_CLASS (penge_email_pane_parent_class)->dispose (object);
}

static void
penge_email_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_email_pane_parent_class)->finalize (object);
}

static void
penge_email_pane_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PengeEmailPanePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_VERTICAL:
      g_value_set_boolean (value, priv->vertical);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_email_pane_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id) {
    case PROP_VERTICAL:
      penge_email_pane_set_vertical (PENGE_EMAIL_PANE (object),
                                     g_value_get_boolean (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_email_pane_class_init (PengeEmailPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeEmailPanePrivate));

  object_class->get_property = penge_email_pane_get_property;
  object_class->set_property = penge_email_pane_set_property;
  object_class->dispose = penge_email_pane_dispose;
  object_class->finalize = penge_email_pane_finalize;

  g_object_class_install_property 
    (object_class, 
     PROP_VERTICAL,
     g_param_spec_boolean ("vertical",
                           "Vertical",
                           "Vertical mode",
                           FALSE,
                           G_PARAM_READWRITE));
}

static void
_tp_provider_prepared (GObject      *source,
                       GAsyncResult *result,
                       gpointer      user_data)
{
  GError *error = NULL;
  if (!mailme_telepathy_prepare_finish (MAILME_TELEPATHY (source), result,
        &error))
  {
    g_warning (G_STRLOC ": Failed to prepare e-mail provider: %s",
               error->message);
    return;
  }
}

static void
_update_count_pane (ClutterActor           *widget,
                    MailmeTelepathyAccount *account)
{
  gchar *display_name = NULL;
  guint unread_count = 0;

  g_object_get (account, "display-name", &display_name,
                         "unread-count", &unread_count,
                         NULL);

  g_object_set (widget, "account", display_name,
                        "count", unread_count,
                        NULL);

  if (unread_count == 0)
    g_object_set (widget, "message", _("No unread messages"), NULL);
  else if (unread_count == 1)
    g_object_set (widget, "message", _("Unread message"), NULL);
  else
    g_object_set (widget, "message", _("Unread messages"), NULL);
}

static void
_account_changed_cb (GObject    *object,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
  GObject *widget = G_OBJECT (user_data);
  _update_count_pane (CLUTTER_ACTOR (widget),
                      MAILME_TELEPATHY_ACCOUNT (object));
}

static void
_received_inbox_open_info_cb (GObject      *source,
                              GAsyncResult *result,
                              gpointer      user_data)
{
  GError *error = NULL;
  MailmeInboxOpenFormat format;
  gchar *value;
  ClutterActor *actor = CLUTTER_ACTOR (user_data);

  value = mailme_telepathy_account_get_inbox_finish (
                                        MAILME_TELEPATHY_ACCOUNT (source),
                                        result,
                                        &format,
                                        &error);

  if (error)
  {
    g_warning ("Failed to get inbox information: %s", error->message);
    g_error_free (error);
    return;
  }

  switch (format)
  {
    case MAILME_INBOX_URI:
      if (!penge_utils_launch_for_uri (actor, value))
      {
        g_warning (G_STRLOC ": Error launching: %s", value);
      } else {
        penge_utils_signal_activated (actor);
      }
      break;

    case MAILME_INBOX_COMMAND_LINE:
      if (!penge_utils_launch_by_command_line (actor, value))
      {
        g_warning (G_STRLOC ": Error launching: %s", value);
      } else {
        penge_utils_signal_activated (actor);
      }
      break;

    default:
      g_warning ("Unknown inbox open format.");
  }
}


static void
_account_button_clicked_cb (MxButton *button,
                            gpointer  user_data)
{
  MailmeTelepathyAccount *account = MAILME_TELEPATHY_ACCOUNT (user_data);

  mailme_telepathy_account_get_inbox_async (
      account,
      _received_inbox_open_info_cb,
      button);
}

static void
_account_added_cb (MailmeTelepathy        *provider,
                   MailmeTelepathyAccount *account,
                   gpointer                user_data)
{
  PengeEmailPanePrivate *priv = GET_PRIVATE (user_data);
  ClutterActor *widget;

  widget = g_object_new (PENGE_TYPE_COUNT_TILE, NULL);
  _update_count_pane (widget, account);

  g_object_set (widget, "compact", priv->vertical, NULL);

  /* FIXME Maybe we want alphabetic order ? */
  clutter_container_add_actor (CLUTTER_CONTAINER (user_data),
                               widget);
  clutter_actor_show (widget);

  g_hash_table_insert (priv->account_to_widget, account, widget);

  g_signal_connect (G_OBJECT (account),
                    "notify::unread-count",
                    G_CALLBACK (_account_changed_cb),
                    widget);

  g_signal_connect (G_OBJECT (account),
                    "notify::display-name",
                    G_CALLBACK (_account_changed_cb),
                    widget);

  g_signal_connect (G_OBJECT (widget),
                    "clicked",
                    G_CALLBACK (_account_button_clicked_cb),
                    account);
}

static void
_account_removed_cb (MailmeTelepathy        *provider,
                     MailmeTelepathyAccount *account,
                     gpointer                user_data)
{
  PengeEmailPanePrivate *priv = GET_PRIVATE (user_data);
  ClutterActor *widget = g_hash_table_lookup (priv->account_to_widget,
                                              account);

  if (widget != NULL)
  {
    g_signal_handlers_disconnect_by_func (account,
        _account_changed_cb,
        widget);

    g_hash_table_remove (priv->account_to_widget, account);

    clutter_container_remove_actor (CLUTTER_CONTAINER (user_data),
                                    widget);
  }
}

static void
penge_email_pane_init (PengeEmailPane *self)
{
  PengeEmailPanePrivate *priv = GET_PRIVATE_REAL (self);

  self->priv = priv;

  priv->account_to_widget = g_hash_table_new (NULL, NULL);
  priv->provider = g_object_new (MAILME_TYPE_TELEPATHY, NULL);
  priv->vertical = FALSE;

  mailme_telepathy_prepare_async (priv->provider,
                                  _tp_provider_prepared,
                                  self);

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self),
                                 MX_ORIENTATION_VERTICAL);

  g_signal_connect (G_OBJECT (priv->provider),
                    "account-added",
                    G_CALLBACK (_account_added_cb),
                    self);

  g_signal_connect (G_OBJECT (priv->provider),
                    "account-removed",
                    G_CALLBACK (_account_removed_cb),
                    self);
}

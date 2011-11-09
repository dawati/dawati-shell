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

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/proxy-subclass.h>

#include "mailme-telepathy-account.h"

G_DEFINE_TYPE (MailmeTelepathyAccount, mailme_telepathy_account, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MAILME_TYPE_TELEPATHY_ACCOUNT,\
                                  MailmeTelepathyAccountPrivate))


#define MAILME_ERRORS mailme_errors_quark ()

enum
{
  PROP_0,
  PROP_DISPLAY_NAME,
  PROP_UNREAD_COUNT,
  PROP_STATUS,
};

typedef struct _MailmeTelepathyAccountPrivate MailmeTelepathyAccountPrivate;

struct _MailmeTelepathyAccountPrivate {
  TpAccount *account;
  MailmeAccountStatus status;
  DBusGProxy *mail_proxy;
  guint unread_count;
  gchar *email_addr;
};

struct _InboxOpenInfo
{
  MailmeInboxOpenFormat format;
  gchar *value;
};

static GQuark
mailme_errors_quark ()
{
  static GQuark quark = 0;
  if (G_UNLIKELY (quark == 0))
  {
    quark = g_quark_from_static_string ("mailme_errors");
  }
  return quark;
}

static gboolean
check_support_and_save (MailmeTelepathyAccountPrivate *priv,
                        GHashTable                    *out_Properties)
{
  guint capabilities;

  /* Here we can ignore the validity flag since 0 will lead to unsupported
   * status */
  capabilities = tp_asv_get_uint32 (out_Properties, "MailNotificationFlags", NULL);
  if ((capabilities & TP_MAIL_NOTIFICATION_FLAG_SUPPORTS_UNREAD_MAIL_COUNT)
      && (capabilities & TP_MAIL_NOTIFICATION_FLAG_SUPPORTS_REQUEST_INBOX_URL))
  {
    gboolean valid = FALSE;
    priv->unread_count = tp_asv_get_uint32 (out_Properties,
                                            "UnreadMailCount",
                                            &valid);
    if (!valid)
      return FALSE;

    priv->status = MAILME_ACCOUNT_SUPPORTED;
  }
  else
  {
    priv->status = MAILME_ACCOUNT_UNSUPPORTED;
  }
  return TRUE;
}

static void
on_mail_notification_get_all (TpProxy      *proxy,
                              GHashTable   *out_Properties,
                              const GError *error,
                              gpointer      user_data,
                              GObject      *weak_object)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
  MailmeTelepathyAccount *self = MAILME_TELEPATHY_ACCOUNT (
      g_async_result_get_source_object (G_ASYNC_RESULT (user_data)));
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (self);

  if (error != NULL)
  {
    g_simple_async_result_set_from_error (simple, error);
    goto complete;
  }

  if (!check_support_and_save (priv, out_Properties))
  {
      GError cm_error = { MAILME_ERRORS, 0,
        "Connection Manager failed to provide an unread count."};
      g_simple_async_result_set_from_error (simple, &cm_error);
  }

complete:
  g_simple_async_result_complete (simple);
  g_object_unref (simple);
  g_object_unref (self);
}

static void
on_unread_mails_changed (DBusGProxy *proxy G_GNUC_UNUSED,
                         guint       count,
                         GPtrArray  *mail_added G_GNUC_UNUSED,
                         gchar     **mail_removed G_GNUC_UNUSED,
                         gpointer    user_data)
{
  guint old_count;
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (user_data);

  old_count = priv->unread_count;
  priv->unread_count = count;

  if (old_count != count)
    g_object_notify (G_OBJECT (user_data), "unread-count");
}

static gboolean
setup_mail_proxy (MailmeTelepathyAccount *self,
                  TpConnection           *connection)
{
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (self);

  if (priv->mail_proxy == NULL)
  {
    GError *error = NULL;

    priv->mail_proxy = tp_proxy_borrow_interface_by_id (
        TP_PROXY (connection),
        TP_IFACE_QUARK_CONNECTION_INTERFACE_MAIL_NOTIFICATION,
        &error);

    if (priv->mail_proxy == NULL)
    {
      /* This is not an error, it only means it's not supported. */
      priv->status = MAILME_ACCOUNT_UNSUPPORTED;
      g_object_notify (G_OBJECT (self), "status");
      return FALSE;
    }

    dbus_g_proxy_connect_signal (priv->mail_proxy,
                                 "UnreadMailsChanged",
                                 G_CALLBACK (on_unread_mails_changed),
                                 self,
                                 NULL);
  }

  return TRUE;
}

static void
on_connection_ready (TpConnection *connection,
    const GError *ready_error,
    gpointer user_data)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
  MailmeTelepathyAccount *self = MAILME_TELEPATHY_ACCOUNT (
      g_async_result_get_source_object (G_ASYNC_RESULT (user_data)));

  if (ready_error != NULL)
  {
    g_simple_async_result_set_from_error (simple, ready_error);
    goto complete;
  }

  tp_connection_add_client_interest (connection,
      TP_IFACE_CONNECTION_INTERFACE_MAIL_NOTIFICATION);
  if (setup_mail_proxy (self, connection))
  {
    tp_cli_dbus_properties_call_get_all (
        connection,
        -1 /* Default timeout */,
        TP_IFACE_CONNECTION_INTERFACE_MAIL_NOTIFICATION,
        on_mail_notification_get_all,
        simple,
        NULL, NULL);
    g_object_unref (self);
    return;
  }

complete:
  g_simple_async_result_complete (simple);
  g_object_unref (simple);
  g_object_unref (self);
}

static void
on_account_prepared (GObject      *source,
                     GAsyncResult *result,
                     gpointer      user_data)
{
  GError *error = NULL;
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
  MailmeTelepathyAccount *self = MAILME_TELEPATHY_ACCOUNT (
    g_async_result_get_source_object (G_ASYNC_RESULT (user_data)));
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (self);

  if (!tp_account_prepare_finish (priv->account, result, &error))
  {
    g_simple_async_result_set_from_error (simple, error);
    goto complete;
  }

  switch (tp_account_get_connection_status (priv->account, NULL))
  {
    case TP_CONNECTION_STATUS_CONNECTED:
      {
        TpConnection *connection = tp_account_get_connection (priv->account);
        /* We must make sure the connection is ready */
        tp_connection_call_when_ready (connection,
                                       on_connection_ready,
                                       user_data);
        g_object_unref (self);
        return;
      }
    default:
      /* Don't assume anything before an account is connected */
      break;
  }

complete:
  g_simple_async_result_complete (simple);
  g_object_unref (simple);
  g_object_unref (self);
}

static void
on_display_name_changed  (GObject    *object,
                          GParamSpec *pspec,
                          gpointer    user_data)
{
  g_object_notify (G_OBJECT (user_data), "display-name");
}

static void
on_mail_notification_get_all_again (TpProxy      *proxy,
                                    GHashTable   *out_Properties,
                                    const GError *error,
                                    gpointer      user_data,
                                    GObject      *weak_object)
{
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (user_data);

  /* On error we simply stay disconnected */
  if (error != NULL)
    return;

  if (check_support_and_save (priv, out_Properties))
    g_object_notify (G_OBJECT (user_data), "status");
}

static void
on_connection_ready_again (TpConnection *connection,
                           const GError *ready_error,
                           gpointer      user_data)
{
  MailmeTelepathyAccount *self = MAILME_TELEPATHY_ACCOUNT (user_data);

  if (ready_error != NULL)
    return;

  tp_connection_add_client_interest (connection,
      TP_IFACE_CONNECTION_INTERFACE_MAIL_NOTIFICATION);
  setup_mail_proxy (self, connection);
  tp_cli_dbus_properties_call_get_all (
      connection,
      -1 /* Default timeout */,
      TP_IFACE_CONNECTION_INTERFACE_MAIL_NOTIFICATION,
      on_mail_notification_get_all_again,
      self,
      NULL, NULL);
}

static void
on_account_status_changed (TpAccount  *account,
                           guint       old_status,
                           guint       new_status,
                           guint       reason,
                           gchar      *dbus_error_name,
                           GHashTable *details,
                           gpointer    user_data)
{
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (user_data);

  if ((old_status != TP_CONNECTION_STATUS_CONNECTED)
      && (new_status == TP_CONNECTION_STATUS_CONNECTED))
  {
    TpConnection *connection = tp_account_get_connection (priv->account);
    tp_connection_call_when_ready (connection,
                                   on_connection_ready_again,
                                   user_data);
  }
  else
  {
    if (priv->mail_proxy)
    {
      priv->mail_proxy = NULL;
    }

    priv->unread_count = 0;

    if (priv->status != MAILME_ACCOUNT_DISCONNECTED)
    {
      priv->status = MAILME_ACCOUNT_DISCONNECTED;
      g_object_notify (G_OBJECT (user_data), "status");
    }
  }
}

static void
_inbox_open_info_free (struct _InboxOpenInfo *inbox)
{
  if (inbox)
  {
    g_free (inbox->value);
    g_free (inbox);
  }
}

static void
_unlink_tmp_html_file (gpointer user_data)
{
  gchar *file_uri = user_data;
  g_unlink (file_uri + 7);
  g_free (file_uri);
}

static gboolean
_unlink_tmp_html_file_cb (gpointer user_data)
{
  /* The GDestroyNotify function attached with this timeout will deal with
   * freeing everything. */
  return FALSE;
}

static gchar *
_create_html_file_and_timeout (const char *url,
                               GPtrArray *post_data,
                               GError **error)
{
  const char *html_part1 =
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
    "  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
    "<head>\n"
    "  <title>MyZone Mailme redirect page</title>\n"
    "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n"
    "</head>\n"
    "<body onload=\"submitForm()\">\n"
    "  <form method=\"post\" action=\"";
  const char *html_part2 = "\" name=\"myForm\" id=\"myForm\">\n";
  const char *html_part3 =
    "  </form>\n"
    "  <script type=\"text/javascript\">document.myForm.submit();</script>\n"
    "</body>\n"
    "</html>";
  gchar *file_uri, *escaped_str;;
  gint fd;
  guint i;

  file_uri = g_strdup ("file:///tmp/mailme-XXXXXX.html");

  /* g_mkstemp need a unix filename, so we skip strlen ("file://"), which is
   * 7 characters, and then pass it to mkstemp that will replace the XXXXXX with the
   * choosen name for the returned tempory file. */
  fd = g_mkstemp (file_uri + 7);

  if (fd < 0)
    goto io_error;

  if (write (fd, html_part1, strlen (html_part1)) < 0)
    goto io_error;

  escaped_str = g_markup_escape_text (url, -1);

  if (write (fd, url, strlen (url)) < 0)
  {
    g_free (escaped_str);
    goto io_error;
  }

  g_free (escaped_str);

  if (write (fd, html_part2, strlen (html_part2)) < 0)
    goto io_error;

  for (i = 0; i < post_data->len; i++)
  {
    GValueArray *item = g_ptr_array_index (post_data, i);
    const gchar *key = g_value_get_string (g_value_array_get_nth (item, 0));
    const gchar *value = g_value_get_string (g_value_array_get_nth (item, 1));
    gchar *html_input;

    html_input = g_markup_printf_escaped (
        "    <input type=\"hidden\" name=\"%s\" value=\"%s\" />\n",
        key,
        value);

    if (write (fd, html_input, strlen (html_input)) < 0)
    {
      g_free (html_input);
      goto io_error;
    }
  }

  if (write (fd, html_part3, strlen (html_part3)) < 0)
    goto io_error;

  g_timeout_add_full (G_PRIORITY_DEFAULT,
                      30000,
                      _unlink_tmp_html_file_cb,
                      g_strdup (file_uri),
                      _unlink_tmp_html_file);

  close (fd);
  return file_uri;

io_error:
  *error = g_error_new (G_IO_ERROR,
                        g_io_error_from_errno (errno),
                        "Failed building redirecting HTML: %s",
                        strerror (errno));
  close (fd);
  _unlink_tmp_html_file (file_uri);

  return NULL;
}

static void
on_received_inbox_url (DBusGProxy       *proxy,
                       DBusGProxyCall   *call_id,
                       void             *user_data)
{
  GError *error = NULL;
  GSimpleAsyncResult *async_result = G_SIMPLE_ASYNC_RESULT (user_data);
  GValueArray *result;
  const gchar *url;
  guint method;
  GPtrArray *post_data;
  struct _InboxOpenInfo *inbox = NULL;

  dbus_g_proxy_end_call (proxy, call_id, &error,
      dbus_g_type_get_struct ("GValueArray",
        G_TYPE_STRING,
        G_TYPE_UINT,
        dbus_g_type_get_collection ("GPtrArray",
          dbus_g_type_get_struct ("GValueArray",
            G_TYPE_STRING,
            G_TYPE_STRING,
            G_TYPE_INVALID)),
        G_TYPE_INVALID),
      &result,
      G_TYPE_INVALID);

  if (error)
  {
    g_simple_async_result_set_from_error (async_result, error);
    g_error_free (error);
    goto complete;
  }

  url = g_value_get_string (g_value_array_get_nth (result, 0));
  method = g_value_get_uint (g_value_array_get_nth (result, 1));
  post_data = g_value_get_boxed (g_value_array_get_nth (result, 2));

  inbox = g_new0 (struct _InboxOpenInfo, 1);

  switch (method)
  {
    case 0: /* Method GET */
      inbox->format = MAILME_INBOX_URI;
      inbox->value = g_strdup (url);
      break;

    case 1: /* Method POST */
      {
        GError *error = NULL;
        gchar *uri;

        uri = _create_html_file_and_timeout (url, post_data, &error);

        if (error)
        {
          g_simple_async_result_set_from_error (async_result, error);
          g_error_free (error);
          g_free (inbox);
          inbox = NULL;
        }
        else
        {
          inbox->format = MAILME_INBOX_URI;
          inbox->value = uri;
        }
      }
      break;

    default:
      {
        GError cm_error = { MAILME_ERRORS, 0,
          "Connection Manager sent and invalid HTTP method value."};
        g_simple_async_result_set_from_error (async_result, &cm_error);
        g_free (inbox);
        inbox = NULL;
      }
  }

  g_value_array_free (result);

complete:
  g_simple_async_result_set_op_res_gpointer (
      async_result,
      inbox,
      (GDestroyNotify)_inbox_open_info_free);

  g_simple_async_result_complete (async_result);
  g_object_unref (async_result);
}

static void
mailme_telepathy_account_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_DISPLAY_NAME:
      g_value_set_string (value, tp_account_get_display_name(priv->account));
      break;
    case PROP_UNREAD_COUNT:
      g_value_set_uint (value, priv->unread_count);
      break;
    case PROP_STATUS:
      g_value_set_int (value, priv->status);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        }
}

static void
mailme_telepathy_account_dispose (GObject *object)
{
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (object);

  if (priv->account)
  {
    g_object_unref (priv->account);
    priv->account = NULL;
    priv->mail_proxy = NULL;
  }
  priv->unread_count = 0;
}

static void
mailme_telepathy_account_finalize (GObject *object)
{
  /* nothing to do */
}

static void
mailme_telepathy_account_class_init (MailmeTelepathyAccountClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MailmeTelepathyAccountPrivate));

  object_class->get_property = mailme_telepathy_account_get_property;
  object_class->dispose = mailme_telepathy_account_dispose;
  object_class->finalize = mailme_telepathy_account_finalize;

  g_object_class_install_property (object_class, PROP_DISPLAY_NAME,
      g_param_spec_string ("display-name",
                           "Display Name",
                           "The account display name",
                           NULL,
                           G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_UNREAD_COUNT,
      g_param_spec_uint ("unread-count",
                         "Unread Count",
                         "The count of unread e-mail for this account.",
                         0, G_MAXUINT, 0,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_STATUS,
      g_param_spec_int ("status",
                        "Account Status",
                        "The status of this account.",
                        0, MAILME_ACCOUNT_NUM_STATUS, 0,
                        G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));
}

static void
mailme_telepathy_account_init (MailmeTelepathyAccount *self)
{
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (self);
  priv->account = NULL;
  priv->status = MAILME_ACCOUNT_DISCONNECTED;
  priv->unread_count = 0;
  priv->email_addr = NULL;
}

void
mailme_telepathy_account_prepare_async (MailmeTelepathyAccount *self,
                                        GObject                *account,
                                        GAsyncReadyCallback     callback,
                                        gpointer                user_data)
{
  GSimpleAsyncResult *result;
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (account != NULL);
  priv->account = g_object_ref (account);

  result = g_simple_async_result_new (G_OBJECT (self),
                                      callback, user_data,
                                      mailme_telepathy_account_prepare_finish);

  tp_account_prepare_async (priv->account, NULL, on_account_prepared, result);

  g_signal_connect (G_OBJECT (priv->account), "notify::display-name",
      G_CALLBACK (on_display_name_changed), self);

  g_signal_connect (G_OBJECT (priv->account), "status-changed",
      G_CALLBACK (on_account_status_changed), self);
}

MailmeAccountStatus
mailme_telepathy_account_prepare_finish (MailmeTelepathyAccount  *self,
                                         GAsyncResult            *result,
                                         GError                 **error)
{
  GSimpleAsyncResult *simple;
  MailmeTelepathyAccountPrivate *priv;

  if (!g_simple_async_result_is_valid (result,
                                       G_OBJECT (self),
                                       mailme_telepathy_account_prepare_finish))
    return MAILME_ACCOUNT_UNSUPPORTED;

  simple = G_SIMPLE_ASYNC_RESULT (result);
  priv = GET_PRIVATE (self);

  g_simple_async_result_propagate_error (simple, error);

  return priv->status;
}

GObject *
mailme_telepathy_account_get_tp_account (MailmeTelepathyAccount *self)
{
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (self);
  return G_OBJECT (priv->account);
}

void
mailme_telepathy_account_get_inbox_async (MailmeTelepathyAccount *self,
                                          GAsyncReadyCallback     callback,
                                          gpointer                user_data)
{
  GSimpleAsyncResult *result;
  MailmeTelepathyAccountPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (priv->account != NULL);

  result = g_simple_async_result_new (G_OBJECT (self),
                                      callback, user_data,
                                      mailme_telepathy_account_get_inbox_finish);

  dbus_g_proxy_begin_call (priv->mail_proxy,
                           "RequestInboxURL",
                           on_received_inbox_url,
                           result,
                           NULL,
                           G_TYPE_INVALID);
}

gchar *
mailme_telepathy_account_get_inbox_finish (MailmeTelepathyAccount  *self,
                                           GAsyncResult            *result,
                                           MailmeInboxOpenFormat   *format,
                                           GError                 **error)
{
  GSimpleAsyncResult *simple;
  struct _InboxOpenInfo *inbox;
  gchar *value;

  if (!g_simple_async_result_is_valid (result,
                                   G_OBJECT (self),
                                   mailme_telepathy_account_get_inbox_finish))
    return NULL;

  simple = G_SIMPLE_ASYNC_RESULT (result);

  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;

  inbox = g_simple_async_result_get_op_res_gpointer (simple);
  *format = inbox->format;
  value = inbox->value;
  inbox->value = NULL;

  return value;
}

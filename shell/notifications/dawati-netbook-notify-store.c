/*
 * (C) 2008 OpenedHand Ltd.
 *
 * Author: Ross Burton <ross@openedhand.com>
 *
 * Licensed under the GPL v2 or greater.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dawati-netbook-notify-store.h"
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>
#include <string.h>
#include "../marshal.h"
#include "../dawati-netbook.h"

G_DEFINE_TYPE (DawatiNetbookNotifyStore, dawati_netbook_notify_store, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DAWATI_NETBOOK_TYPE_NOTIFY_STORE, DawatiNetbookNotifyStorePrivate))

enum {
  ACTION_INVOKED,
  NOTIFICATION_ADDED,
  NOTIFICATION_CLOSED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

static DBusConnection *_dbus_conn = NULL;

#define DEFAULT_TIMEOUT 7000

typedef struct {
  guint next_id;
  GList *notifications;
  DBusGProxy *bus_proxy;
} DawatiNetbookNotifyStorePrivate;

static guint
get_next_id (DawatiNetbookNotifyStore *notify)
{
  DawatiNetbookNotifyStorePrivate *priv;

  g_return_val_if_fail (DAWATI_NETBOOK_IS_NOTIFY (notify), 0);

  priv = GET_PRIVATE (notify);

  /* We're not threaded, so this is perfectly safe */
  return ++priv->next_id;
}

static gboolean
find_notification (DawatiNetbookNotifyStore  *notify,
                   guint                      id,
                   Notification             **found)
{
  /* TODO: should this return a GList*? */
  DawatiNetbookNotifyStorePrivate *priv;
  GList *l;

  g_return_val_if_fail (DAWATI_NETBOOK_IS_NOTIFY (notify) && id && found,
                        FALSE);

  priv = GET_PRIVATE (notify);

  for (l = priv->notifications; l ; l = l->next)
    {
      Notification *notification = l->data;
      if (notification->id == id)
        {
          *found = notification;
          return TRUE;
        }
    }

  return FALSE;
}

static void
free_notification (Notification *n)
{
  GList *action;

  g_return_if_fail (n);

  g_free (n->summary);
  g_free (n->body);
  g_free (n->icon_name);

  for (action = n->actions; action; action = g_list_next (action))
    g_free (action->data);
  g_list_free (n->actions);

  if (n->icon_pixbuf)
    g_object_unref (n->icon_pixbuf);

  g_slice_free (Notification, n);
}

/*
 * Notification Manager implementation.
 */

/*
 * Gets notificaiton with given id, creating one if it does not exist.
 * internal_data is data to be passed to internal notificatio callbacks and
 * is NULL for external notifications.
 */
static Notification *
get_notification (DawatiNetbookNotifyStore *notify,
                  guint                     id,
                  gpointer                  internal_data)
{
  DawatiNetbookNotifyStorePrivate *priv;
  Notification                    *notification;
  GList                           *action;

  g_return_val_if_fail (DAWATI_NETBOOK_IS_NOTIFY (notify), NULL);

  priv = GET_PRIVATE (notify);

  if (id && find_notification (notify, id, &notification))
    {
      /* Found an existing notification, clear it */
      g_free (notification->summary);
      notification->summary = NULL;

      g_free (notification->body);
      notification->body = NULL;

      g_free (notification->icon_name);
      notification->icon_name = NULL;

      for (action = notification->actions; action; action = g_list_next(action))
        g_free (action->data);
      g_list_free (notification->actions);
      notification->actions = NULL;
    }
  else
    {
      id = get_next_id (notify);

      g_return_val_if_fail (id, NULL);

      /* This is a new notification, create a new structure and allocate an ID
       */
      notification = g_slice_new0 (Notification);
      notification->id = id;
      notification->internal_data = internal_data;

      /* TODO: use _insert_sorted with some magic sorting algorithm */
      priv->notifications = g_list_append (priv->notifications, notification);
    }

  return notification;
}

typedef struct _PidData
{
  DawatiNetbookNotifyStore  *store;
  guint                     notification_id;
} PidData;

static void
unix_process_id_reply_cb (DBusGProxy *proxy,
                          guint       pid,
                          GError     *error,
                          gpointer    data)
{
  PidData      *pid_data = data;
  Notification *notification;

  g_return_if_fail (data && pid);

  if (find_notification (pid_data->store,
                         pid_data->notification_id,
                         &notification))
    {

      notification->pid = pid;

      g_signal_emit (pid_data->store,
                     signals[NOTIFICATION_ADDED], 0, notification);
    }

  g_slice_free (PidData, pid_data);
}

/*
 * Implementation of the dbus notify method
 */
static gboolean
notification_manager_notify (DawatiNetbookNotifyStore  *notify,
                             const gchar               *app_name,
                             const guint                id,
                             const gchar               *icon,
                             const gchar               *summary,
                             const gchar               *body,
                             const gchar              **actions,
                             GHashTable                *hints,
                             gint                       timeout,
                             DBusGMethodInvocation     *context)
{
  DawatiNetbookNotifyStorePrivate *priv;
  Notification *notification;
  gint          i;

  g_return_val_if_fail (DAWATI_NETBOOK_IS_NOTIFY (notify), FALSE);

  /*
   * We will not bother to show a notification that has neither summary, nor
   * body.
   */
  if (!((summary && *summary) || (body && *body)))
    return FALSE;

  priv = GET_PRIVATE (notify);

  notification = get_notification (notify, id, NULL);

  g_return_val_if_fail (notification, FALSE);

  notification->summary = g_strdup (summary);
  notification->body = g_strdup (body);
  notification->icon_name = g_strdup (icon);

  if (hints)
    {
      GValue *val = NULL;

      val = g_hash_table_lookup (hints, "urgency");
      notification->is_urgent =
        (val && g_value_get_uchar(val) == 2) ? TRUE : FALSE;

      val = g_hash_table_lookup (hints, "dawati-no-dismiss");
      notification->no_dismiss_button =
        (val && g_value_get_uchar (val) > 0) ? TRUE : FALSE;


      val = g_hash_table_lookup (hints, "icon_data");
      if (val && G_VALUE_HOLDS (val, G_TYPE_VALUE_ARRAY))
        {
          GValueArray *array = g_value_get_boxed (val);
          GdkPixbuf   *pixbuf;
          gint width, height, rowstride, bits_per_sample;
          gboolean has_alpha;
          GArray *data_array;
          GValue *v;

          v = g_value_array_get_nth (array, 0);
          width = g_value_get_int (v);

          v = g_value_array_get_nth (array, 1);
          height = g_value_get_int (v);

          v = g_value_array_get_nth (array, 2);
          rowstride = g_value_get_int (v);

          v = g_value_array_get_nth (array, 3);
          has_alpha = g_value_get_boolean (v);

          v = g_value_array_get_nth (array, 4);
          bits_per_sample = g_value_get_int (v);

          v = g_value_array_get_nth (array, 6);
          data_array = g_value_get_boxed (v);

          pixbuf = gdk_pixbuf_new_from_data ((const guchar*) data_array->data,
                                             GDK_COLORSPACE_RGB,
                                             has_alpha,
                                             bits_per_sample,
                                             width,
                                             height,
                                             rowstride,
                                             NULL, NULL);

          if (notification->icon_pixbuf)
            g_object_unref (notification->icon_pixbuf);

          notification->icon_pixbuf = pixbuf;
        }
    }

  for (i = 0; actions[i] && actions[i+1]; i += 2)
    {
      const gchar *action = actions[i];
      const gchar *label  = actions[i + 1];

      /*
       * If either the action or the associated label is an empty string,
       * just skip it.
       */
      if (!*action || !*label)
        continue;

      notification->actions = g_list_append (notification->actions,
                                             g_strdup (action));
      notification->actions = g_list_append (notification->actions,
                                             g_strdup (label));
    }

  /*
   * A timeout of -1 means implementation defined, e.g., default timeout.
   * We do not allow timeouts greater than our default timeout for non-urgent
   * notification (this includes requests for notifications that do not timeout
   * (timeout of 0)
   */
  if (timeout < 0 ||
      (!notification->is_urgent && (timeout == 0 || timeout > DEFAULT_TIMEOUT)))
    timeout = DEFAULT_TIMEOUT;

  notification->timeout_ms = timeout;

  if (context)
    {
      PidData *pid_data = g_slice_new0 (PidData);

      pid_data->store           = notify;
      pid_data->notification_id = notification->id;

      notification->sender = dbus_g_method_get_sender (context);

      org_freedesktop_DBus_get_connection_unix_process_id_async (priv->bus_proxy,
                                                                 notification->sender,
                                                                 unix_process_id_reply_cb,
                                                                 pid_data);
    }
  else
    g_signal_emit (notify, signals[NOTIFICATION_ADDED], 0, notification);

  if (context)
    dbus_g_method_return(context, notification->id);

  return TRUE;
}

/*
 * Function allowing to pop up notifications internally from mutter-dawati
 *
 * This is bit of a hack that relies on special action ids, and has the
 * associated actions hardcoded in dawati_netbook_notify_store_action().
 *
 * data: the data to pass to the specific internal callback.
 */
guint
notification_manager_notify_internal (DawatiNetbookNotifyStore  *notify,
                                      guint                      id,
                                      const gchar               *app_name,
                                      const gchar               *icon,
                                      const gchar               *summary,
                                      const gchar               *body,
                                      const gchar              **actions,
                                      GHashTable                *hints,
                                      gint                       timeout,
                                      gpointer                   data)
{
  Notification *n;

  g_return_val_if_fail (DAWATI_NETBOOK_IS_NOTIFY (notify) && id, 0);

  n = get_notification (notify, id, data);

  g_return_val_if_fail (n && n->internal_data == data, 0);

  notification_manager_notify (notify, app_name, n->id, icon, summary, body,
                               actions, hints, timeout, NULL);

  return n->id;
}

static gboolean
notification_manager_close_notification (DawatiNetbookNotifyStore  *notify,
                                         guint                      id,
                                         GError                   **error)
{
  g_return_val_if_fail (DAWATI_NETBOOK_IS_NOTIFY (notify), FALSE);

  if (dawati_netbook_notify_store_close (notify, id, ClosedProgramatically))
    {
      return TRUE;
    }
  else
    {
      g_set_error (error, g_quark_from_static_string("NotifyStore"), 0,
                   "Unknown notification ID %d", id);
      return FALSE;
    }
}

static gboolean
notification_manager_get_capabilities (DawatiNetbookNotifyStore   *notify,
				       gchar                    ***caps,
                                       GError                     *error)
{
  g_return_val_if_fail (DAWATI_NETBOOK_IS_NOTIFY (notify) && caps, FALSE);

  *caps = g_new0 (gchar *, 7);

  (*caps)[0] = g_strdup ("body");
  (*caps)[1] = g_strdup ("body-markup");
  (*caps)[2] = g_strdup ("summary");
  (*caps)[3] = g_strdup ("icon-static");
  (*caps)[4] = g_strdup ("actions");
  (*caps)[5] = g_strdup ("dawati-no-dismiss");
  (*caps)[6] = NULL;

  return TRUE;
}

static gboolean
notification_manager_get_server_information (DawatiNetbookNotifyStore  *notify,
                                             gchar                    **name,
                                             gchar                    **vendor,
                                             gchar                    **version,
                                             gchar                    **spec_version,
                                             GError                    *error)
{
  g_return_val_if_fail (DAWATI_NETBOOK_IS_NOTIFY (notify) &&
                        name && vendor && version, FALSE);

  *name    = g_strdup ("Dawati Netbook Notification Manager");
  *vendor  = g_strdup ("Dawati Netbook");
  *version = g_strdup (VERSION);
  *spec_version = g_strdup ("1.2");

  return TRUE;
}


/*
 * GObject methods
 */

static void
dawati_netbook_notify_store_finalize (GObject *object)
{
  DawatiNetbookNotifyStorePrivate *priv = GET_PRIVATE (object);

  g_list_foreach (priv->notifications, (GFunc)free_notification, NULL);
  g_list_free (priv->notifications);

  G_OBJECT_CLASS (dawati_netbook_notify_store_parent_class)->finalize (object);
}

#include "notification-manager-glue.h"
static void
dawati_netbook_notify_store_class_init (DawatiNetbookNotifyStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DawatiNetbookNotifyStorePrivate));

  object_class->finalize = dawati_netbook_notify_store_finalize;

  /* TODO: implement ActionInvoked */
  signals[ACTION_INVOKED] =
    g_signal_new ("action-invoked",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (DawatiNetbookNotifyStoreClass,
                                   action_invoked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__UINT_POINTER,
                  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);

  signals[NOTIFICATION_ADDED] =
    g_signal_new ("notification-added",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (DawatiNetbookNotifyStoreClass,
                                   notification_added),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);

  signals[NOTIFICATION_CLOSED] =
    g_signal_new ("notification-closed",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (DawatiNetbookNotifyStoreClass,
                                   notification_closed),
                  NULL, NULL,
                  dawati_netbook_marshal_VOID__UINT_UINT,
                  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_notification_manager_object_info);
}

static void
connect_to_dbus (DawatiNetbookNotifyStore *self)
{
  DawatiNetbookNotifyStorePrivate *priv = GET_PRIVATE (self);
  DBusGConnection *connection;
  GError *error = NULL;
  guint32 request_name_ret;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_warning ("Cannot connect to DBus: %s", error->message);
    g_error_free (error);
    return;
  }

  _dbus_conn = dbus_g_connection_get_connection(connection);

  priv->bus_proxy = dbus_g_proxy_new_for_name (connection,
                                         DBUS_SERVICE_DBUS,
                                         DBUS_PATH_DBUS,
                                         DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (priv->bus_proxy,
                                          "org.freedesktop.Notifications",
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &request_name_ret, &error))
    {
      g_warning ("Cannot request name: %s", error->message);
      g_error_free (error);
      return;
    }

  if (request_name_ret == DBUS_REQUEST_NAME_REPLY_EXISTS)
    {
      g_printerr ("Notification manager already running, not taking over\n");
      return;
    }

  dbus_g_connection_register_g_object (connection,
                                       "/org/freedesktop/Notifications",
                                       G_OBJECT (self));
}

static void
dawati_netbook_notify_store_init (DawatiNetbookNotifyStore *self)
{
  connect_to_dbus (self);
}

DawatiNetbookNotifyStore *
dawati_netbook_notify_store_new (void)
{
  return g_object_new (DAWATI_NETBOOK_TYPE_NOTIFY_STORE, NULL);
}

gboolean
dawati_netbook_notify_store_close (DawatiNetbookNotifyStore           *notify,
                                   guint                               id,
                                   DawatiNetbookNotifyStoreCloseReason reason)
{
  DawatiNetbookNotifyStorePrivate *priv;
  Notification                    *notification;

  g_return_val_if_fail (DAWATI_NETBOOK_IS_NOTIFY (notify), FALSE);

  priv = GET_PRIVATE (notify);

  if (find_notification (notify, id, &notification))
    {
      priv->notifications = g_list_remove (priv->notifications, notification);
      free_notification (notification);
      g_signal_emit (notify, signals[NOTIFICATION_CLOSED], 0, id, reason);

      return TRUE;
    }

  return FALSE;
}

void
dawati_netbook_notify_store_action (DawatiNetbookNotifyStore *notify,
				    guint                     id,
				    gchar                    *action)
{
  Notification *notification;

  g_return_if_fail (DAWATI_NETBOOK_IS_NOTIFY (notify) && id && action);

  if (find_notification (notify, id, &notification))
    {
      if (notification->sender)
        {
          g_signal_emit (notify, signals[ACTION_INVOKED], 0, id, action);
        }
      else
        {
          /*
           * Internal notification
           *
           * TODO -- if we are going to use this for anything else, we should add
           *         api to formally install callbacks.
           */
          if (!strcmp (action, "MNB-urgent-window"))
            {
              dawati_netbook_activate_window (notification->internal_data);
            }
        }
      /* invoke_action_for_notification (notification, action); */
      dawati_netbook_notify_store_close (notify, id, ClosedProgramatically);
    }
}

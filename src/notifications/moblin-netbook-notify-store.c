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

#include "moblin-netbook-notify-store.h"
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>
#include <string.h>
#include "../marshal.h"
#include "../moblin-netbook.h"

G_DEFINE_TYPE (MoblinNetbookNotifyStore, moblin_netbook_notify_store, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MOBLIN_NETBOOK_TYPE_NOTIFY_STORE, MoblinNetbookNotifyStorePrivate))

enum {
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
} MoblinNetbookNotifyStorePrivate;

static guint
get_next_id (MoblinNetbookNotifyStore *notify)
{
  MoblinNetbookNotifyStorePrivate *priv = GET_PRIVATE (notify);
  /* We're not threaded, so this is perfectly safe */
  return ++priv->next_id;
}

static gboolean
find_notification (MoblinNetbookNotifyStore  *notify,
                   guint                      id,
                   Notification             **found)
{
  /* TODO: should this return a GList*? */
  MoblinNetbookNotifyStorePrivate *priv = GET_PRIVATE (notify);
  GList *l;

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
  g_free (n->summary);
  g_free (n->body);
  g_free (n->icon_name);
  g_hash_table_destroy (n->actions);
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
get_notification (MoblinNetbookNotifyStore *notify,
                  guint                     id,
                  gpointer                  internal_data)
{
  MoblinNetbookNotifyStorePrivate *priv = GET_PRIVATE (notify);
  Notification                    *notification;

  if (find_notification (notify, id, &notification))
    {
      /* Found an existing notification, clear it */
      g_free (notification->summary);
      g_free (notification->body);
      g_free (notification->icon_name);
      g_hash_table_remove_all (notification->actions);
    }
  else
    {
      /* This is a new notification, create a new structure and allocate an ID
       */
      notification = g_slice_new0 (Notification);
      notification->id = get_next_id (notify);
      notification->actions = g_hash_table_new_full (NULL,
                                                     NULL,
                                                     g_free,
                                                     g_free);

      notification->internal_data = internal_data;

      /* TODO: use _insert_sorted with some magic sorting algorithm */
      priv->notifications = g_list_append (priv->notifications, notification);
    }

  return notification;
}

/*
 * Implementation of the dbus notify method
 */
static gboolean
notification_manager_notify (MoblinNetbookNotifyStore  *notify,
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
  Notification *notification;
  GValue       *val = NULL;
  gint          i;

  /* TODO: Sanity check the required arguments */
  notification = get_notification (notify, id, NULL);

  notification->summary = g_strdup (summary);
  notification->body = g_strdup (body);
  notification->icon_name = g_strdup (icon);

  if (hints)
    val = g_hash_table_lookup (hints, "urgency");

  notification->is_urgent = (val && g_value_get_uchar(val) == 2) ? TRUE : FALSE;

  for (i = 0; actions[i] != NULL; i += 2)
    {
      const gchar *label = actions[i + 1];

      if (label == NULL || actions[i] == NULL)
	continue;

      g_hash_table_insert (notification->actions,
			   g_strdup (actions[i]),
			   g_strdup (label));
    }

  /* A timeout of -1 means implementation defined */
  if (timeout == -1)
    timeout = DEFAULT_TIMEOUT;

  notification->timeout_ms = timeout;

  if (context)
    notification->sender = dbus_g_method_get_sender (context);

  g_signal_emit (notify, signals[NOTIFICATION_ADDED], 0, notification);

  if (context)
    dbus_g_method_return(context, notification->id);

  return TRUE;
}

/*
 * Function allowing to pop up notifications internally from mutter-moblin
 *
 * This is bit of a hack that relies on special action ids, and has the
 * associated actions hardcoded in invoke_action_for_notification().
 *
 * data: the data to pass to the specific internal callback.
 */
guint
notification_manager_notify_internal (MoblinNetbookNotifyStore  *notify,
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
  Notification *n = get_notification (notify, id, data);

  g_assert (n->internal_data == data);

  notification_manager_notify (notify, app_name, n->id, icon, summary, body,
                               actions, hints, timeout, NULL);

  return n->id;
}

static gboolean
notification_manager_close_notification (MoblinNetbookNotifyStore  *notify,
                                         guint                      id,
                                         GError                   **error)
{
  if (moblin_netbook_notify_store_close (notify, id, ClosedProgramatically))
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
notification_manager_get_capabilities (MoblinNetbookNotifyStore   *notify,
				       gchar                    ***caps,
                                       GError                     *error)
{
  *caps = g_new0 (gchar *, 6);

  (*caps)[0] = g_strdup ("body");
  (*caps)[1] = g_strdup ("body-markup");
  (*caps)[2] = g_strdup ("summary");
  (*caps)[3] = g_strdup ("icon-static");
  (*caps)[4] = g_strdup ("actions");
  (*caps)[5] = NULL;

  return TRUE;
}

static gboolean
notification_manager_get_server_information (MoblinNetbookNotifyStore  *notify,
                                             gchar                    **name,
                                             gchar                    **vendor,
                                             gchar                    **version,
                                             GError                    *error)
{
  *name    = g_strdup ("Moblin Netbook Notification Manager");
  *vendor  = g_strdup ("Moblin Netbook");
  *version = g_strdup (VERSION);

  return TRUE;
}


/*
 * GObject methods
 */

static void
moblin_netbook_notify_store_finalize (GObject *object)
{
  MoblinNetbookNotifyStorePrivate *priv = GET_PRIVATE (object);

  g_list_foreach (priv->notifications, (GFunc)free_notification, NULL);
  g_list_free (priv->notifications);

  G_OBJECT_CLASS (moblin_netbook_notify_store_parent_class)->finalize (object);
}

#include "notification-manager-glue.h"
static void
moblin_netbook_notify_store_class_init (MoblinNetbookNotifyStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MoblinNetbookNotifyStorePrivate));

  object_class->finalize = moblin_netbook_notify_store_finalize;

  signals[NOTIFICATION_ADDED] =
    g_signal_new ("notification-added",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MoblinNetbookNotifyStoreClass,
                                   notification_added),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);

  signals[NOTIFICATION_CLOSED] =
    g_signal_new ("notification-closed",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MoblinNetbookNotifyStoreClass,
                                   notification_closed),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__UINT_UINT,
                  G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_notification_manager_object_info);
}

static void
connect_to_dbus (MoblinNetbookNotifyStore *self)
{
  DBusGConnection *connection;
  DBusGProxy *bus_proxy;
  GError *error = NULL;
  guint32 request_name_ret;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_warning ("Cannot connect to DBus: %s", error->message);
    g_error_free (error);
    return;
  }

  _dbus_conn = dbus_g_connection_get_connection(connection);

  bus_proxy = dbus_g_proxy_new_for_name (connection,
                                         DBUS_SERVICE_DBUS,
                                         DBUS_PATH_DBUS,
                                         DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (bus_proxy,
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
moblin_netbook_notify_store_init (MoblinNetbookNotifyStore *self)
{
  connect_to_dbus (self);
}

static DBusMessage*
create_signal_for_notification (Notification *n, const char *signal_name)
{
  DBusMessage *message;

  message = dbus_message_new_signal("/org/freedesktop/Notifications",
				    "org.freedesktop.Notifications",
				    signal_name);

  dbus_message_set_destination(message, n->sender);
  dbus_message_append_args(message,
			   DBUS_TYPE_UINT32, &n->id,
			   DBUS_TYPE_INVALID);

  return message;
}

static void
invoke_action_for_notification (Notification *n, const char *key)
{
  DBusMessage *message;

  if (n->sender)
    {
      message = create_signal_for_notification (n, "ActionInvoked");
      dbus_message_append_args(message,
                               DBUS_TYPE_STRING, &key,
                               DBUS_TYPE_INVALID);

      dbus_connection_send(_dbus_conn, message, NULL);
      dbus_message_unref(message);
    }
  else
    {
      /*
       * Internal notification
       *
       * TODO -- if we are going to use this for anything else, we should add
       *         api to formally install callbacks.
       */
      if (!strcmp (key, "MNB-urgent-window"))
        {
          moblin_netbook_activate_window (n->internal_data);
        }
    }
}

MoblinNetbookNotifyStore *
moblin_netbook_notify_store_new (void)
{
  return g_object_new (MOBLIN_NETBOOK_TYPE_NOTIFY_STORE, NULL);
}

gboolean
moblin_netbook_notify_store_close (MoblinNetbookNotifyStore           *notify,
                                   guint                               id,
                                   MoblinNetbookNotifyStoreCloseReason reason)
{
  MoblinNetbookNotifyStorePrivate *priv = GET_PRIVATE (notify);
  Notification *notification;

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
moblin_netbook_notify_store_action (MoblinNetbookNotifyStore *notify,
				    guint                     id,
				    gchar                    *action)
{
  Notification *notification;

  if (find_notification (notify, id, &notification))
    {
      invoke_action_for_notification (notification, action);
      moblin_netbook_notify_store_close (notify, id, ClosedProgramatically);
    }
}

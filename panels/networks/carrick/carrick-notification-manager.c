/* carrick-notification-manager.c */

#include "carrick-notification-manager.h"

#include <config.h>
#include <libnotify/notify.h>
#include <glib/gi18n.h>

#include "carrick-icon-factory.h"

G_DEFINE_TYPE (CarrickNotificationManager, carrick_notification_manager, G_TYPE_OBJECT)

#define NOTIFICATION_MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_NOTIFICATION_MANAGER, CarrickNotificationManagerPrivate))

struct _CarrickNotificationManagerPrivate
{
  CmManager *manager;
  gchar *last_type;
  gchar *last_name;
  gchar *last_state;
};

enum
{
  PROP_0,
  PROP_MANAGER
};

void
carrick_notification_manager_queue_service (CarrickNotificationManager *self,
                                            CmService *service)
{
}

static void
_services_changed_cb (CmManager *manager,
                      CarrickNotificationManager *self)
{
  CarrickNotificationManagerPrivate *priv = self->priv;
  const GList *new_services = cm_manager_get_services (manager);
  CmService *new_active = new_services->data;
  NotifyNotification *note;
  gchar *title = NULL;
  gchar *message = NULL;
  const gchar *icon = NULL;
  const gchar *type = NULL;
  const gchar *name = NULL;
  const gchar *state = NULL;
  guint str = 0;

  g_return_if_fail (new_active != NULL);

  type = cm_service_get_type (new_active);
  name = cm_service_get_name (new_active);
  state = cm_service_get_state (new_active);

  /* Don't show on startup */
  /* FIXME: only show for non-user action */

  /* determine whether to show a notification */
  if (g_strcmp0 (priv->last_state, state) != 0 ||
      (g_strcmp0 (priv->last_type, type) != 0 &&
       g_strcmp0 (priv->last_name, name) != 0))
  {
    if (g_strcmp0 (state, "ready") == 0)
    {
      title = g_strdup (_("Connection found"));

      if (g_strcmp0 (type, "ethernet") == 0)
      {
        if (!icon)
        {
          icon = carrick_icon_factory_get_path_for_state (ICON_ACTIVE);
        }

        message = g_strdup_printf (_("You are now connected to a wired network"));
      }
      else
      {
        if (!icon)
        {
          str = cm_service_get_strength (new_active);
          if (g_strcmp0 (type, "wifi") == 0)
          {
            if (str > 70)
              icon = carrick_icon_factory_get_path_for_state (ICON_WIRELESS_STRONG);
            else if (str > 35)
              icon = carrick_icon_factory_get_path_for_state (ICON_WIRELESS_GOOD);
            else
              icon = carrick_icon_factory_get_path_for_state (ICON_WIRELESS_WEAK);
          }
          else if (g_strcmp0 (type, "wimax") == 0)
          {
            if (str > 50)
              icon = carrick_icon_factory_get_path_for_state (ICON_WIMAX_STRONG);
            else
              icon = carrick_icon_factory_get_path_for_state (ICON_WIMAX_WEAK);
          }
          else if (g_strcmp0 (type, "cellular") == 0)
          {
            if (str > 50)
              icon = carrick_icon_factory_get_path_for_state (ICON_3G_STRONG);
            else
              icon = carrick_icon_factory_get_path_for_state (ICON_3G_WEAK);
          }
        }

        if (name && name[0] != '\0')
        {
          message = g_strdup_printf (_("You are now connected to %s network %s"),
                                     type,
                                     name);
        }
        else
        {
          message = g_strdup_printf (_("You are now connected to %s network"),
                                     type);
        }
      }
    }
    else if (g_strcmp0 (state, "idle") == 0)
    {
      title = g_strdup (_("Connection lost"));
      icon = carrick_icon_factory_get_path_for_state (ICON_OFFLINE);

      if (priv->last_name)
      {
        message = g_strdup_printf (_("Your %s connection to %s has been lost"),
                                   priv->last_type,
                                   priv->last_name);
      }
      else if (priv->last_type)
      {
        message = g_strdup_printf (_("Your %s connection has been lost"),
                                   priv->last_type);
      }
      else
        return;
    }
    else
    {
      return;
    }

    /* Now that it's all set up, show the notification */
    note = notify_notification_new (title,
                                    message,
                                    icon,
                                    NULL);
    notify_notification_set_timeout (note,
                                     10000);
    notify_notification_show (note,
                              NULL);
    g_object_unref (note);

    g_free (title);
    g_free (message);
  }

  /* Need to remember this service as last_active */

  g_free (priv->last_state);
  priv->last_state = g_strdup (state);
  g_free (priv->last_type);
  priv->last_type = g_strdup (type);
  g_free (priv->last_name);
  priv->last_name = g_strdup (name);
}

static void
_set_manager (CarrickNotificationManager *self,
              CmManager *manager)
{
  CarrickNotificationManagerPrivate *priv = self->priv;

  if (priv->manager)
  {
    g_signal_handlers_disconnect_by_func (priv->manager,
                                          _services_changed_cb,
                                          self);
    g_object_unref (priv->manager);
    priv->manager = NULL;
  }

  if (manager)
  {
    priv->manager = g_object_ref (manager);
    g_signal_connect (priv->manager,
                      "services-changed",
                      G_CALLBACK (_services_changed_cb),
                      self);
  }
}

static void
carrick_notification_manager_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CarrickNotificationManager *notif = CARRICK_NOTIFICATION_MANAGER (object);
  CarrickNotificationManagerPrivate *priv = notif->priv;

  switch (property_id)
  {
  case PROP_MANAGER:
    g_value_set_object (value, priv->manager);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
carrick_notification_manager_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  CarrickNotificationManager *notif = CARRICK_NOTIFICATION_MANAGER (object);

  switch (property_id)
  {
  case PROP_MANAGER:
    _set_manager (notif,
                  CM_MANAGER (g_value_get_object (value)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
carrick_notification_manager_dispose (GObject *object)
{

  CarrickNotificationManager *self = CARRICK_NOTIFICATION_MANAGER (object);
  CarrickNotificationManagerPrivate *priv = self->priv;

  notify_uninit ();

  if (priv->manager)
  {
    _set_manager (self,
                  NULL);
  }

  G_OBJECT_CLASS (carrick_notification_manager_parent_class)->dispose (object);
}

static void
carrick_notification_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (carrick_notification_manager_parent_class)->finalize (object);
}

static void
carrick_notification_manager_class_init (CarrickNotificationManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (CarrickNotificationManagerPrivate));

  object_class->get_property = carrick_notification_manager_get_property;
  object_class->set_property = carrick_notification_manager_set_property;
  object_class->dispose = carrick_notification_manager_dispose;
  object_class->finalize = carrick_notification_manager_finalize;

  pspec = g_param_spec_object ("manager",
                               "CmManager",
                               "CmManager object to watch for changed services",
                               CM_TYPE_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_MANAGER,
                                   pspec);
}

static void
carrick_notification_manager_init (CarrickNotificationManager *self)
{
  self->priv = NOTIFICATION_MANAGER_PRIVATE (self);

  self->priv->manager = NULL;
  self->priv->last_type = NULL;
  self->priv->last_name = NULL;
  self->priv->last_state = NULL;

  notify_init ("Carrick");
}

CarrickNotificationManager *
carrick_notification_manager_new (CmManager *manager)
{
  return g_object_new (CARRICK_TYPE_NOTIFICATION_MANAGER,
                       "manager",
                       manager,
                       NULL);
}

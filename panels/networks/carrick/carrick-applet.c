/* carrick-applet.c */

#include "carrick-applet.h"

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gconnman/gconnman.h>
#include <libnotify/notify.h>
#include "carrick-pane.h"
#include "carrick-status-icon.h"
#include "carrick-list.h"
#include "carrick-icon-factory.h"

G_DEFINE_TYPE (CarrickApplet, carrick_applet, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_APPLET, CarrickAppletPrivate))

typedef struct _CarrickAppletPrivate CarrickAppletPrivate;

struct _CarrickAppletPrivate {
  CmManager          *manager;
  GtkWidget          *icon;
  GtkWidget          *pane;
  gchar              *state;
  CarrickIconFactory *icon_factory;
};

void
_notify_connection_changed (CarrickApplet *self)
{
  NotifyNotification *note;
  GError *error = NULL;
  gchar *title;
  gchar *message;
  const gchar *icon; // filename
  CarrickAppletPrivate *priv = GET_PRIVATE (self);

  if (priv->state &&
      g_strcmp0 ("offline", priv->state) == 0)
  {
    title = g_strdup_printf (_("Offline"));
    message = g_strdup_printf (_("No active connection."));
  }
  else
  {
    /* FIXME: should probably handle NULL active */
    CmConnection *active = cm_manager_get_active_connection (priv->manager);
    CmConnectionType type = cm_connection_get_type (active);
    title = g_strdup_printf (_("Online"));
    message = g_strdup_printf (_("Now connected to %s."),
                               cm_connection_type_to_string (type));
  }

  icon = carrick_icon_factory_get_path_for_service
    (cm_manager_get_active_service (priv->manager));

  note = notify_notification_new (title,
                                  message,
                                  icon,
                                  NULL);
  notify_notification_set_timeout (note,
                                   3000); // FIXME: 3s

  notify_notification_show (note,
                            &error);
  if (error) {
    g_debug ("Error sending notification: %s",
             error->message);
  }
  g_object_unref (note);
}

static void
manager_connections_changed_cb (CmManager *manager,
                                gpointer   user_data)
{
  CarrickApplet *applet = CARRICK_APPLET (user_data);
  CarrickAppletPrivate *priv = GET_PRIVATE (applet);

  carrick_status_icon_update (CARRICK_STATUS_ICON (priv->icon));
}

static void
manager_state_changed_cb (CmManager *manager,
                          gpointer   user_data)
{
  CarrickApplet *applet = CARRICK_APPLET (user_data);
  CarrickAppletPrivate *priv = GET_PRIVATE (applet);
  gchar *new_state = g_strdup (cm_manager_get_state (manager));

  if (g_strcmp0 (priv->state, new_state) != 0)
  {
    g_free (priv->state);
    priv->state = new_state;
  }

  carrick_status_icon_update (CARRICK_STATUS_ICON (priv->icon));
  _notify_connection_changed (applet);
}

GtkWidget*
carrick_applet_get_pane (CarrickApplet *applet)
{
  CarrickAppletPrivate *priv = GET_PRIVATE (applet);

  return priv->pane;
}

GtkWidget*
carrick_applet_get_icon (CarrickApplet *applet)
{
  CarrickAppletPrivate *priv = GET_PRIVATE (applet);

  return priv->icon;
}

static void
carrick_applet_dispose (GObject *object)
{
  notify_uninit ();
  G_OBJECT_CLASS (carrick_applet_parent_class)->dispose (object);
}

static void
carrick_applet_finalize (GObject *object)
{
  G_OBJECT_CLASS (carrick_applet_parent_class)->finalize (object);
}

static void
carrick_applet_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_applet_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_applet_class_init (CarrickAppletClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CarrickAppletPrivate));

  object_class->dispose = carrick_applet_dispose;
  object_class->finalize = carrick_applet_finalize;
  object_class->get_property = carrick_applet_get_property;
  object_class->set_property = carrick_applet_set_property;
}

static void
carrick_applet_init (CarrickApplet *self)
{
  CarrickAppletPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;
  GtkWidget *scroll_view;

  notify_init ("Carrick");

  scroll_view = gtk_scrolled_window_new (NULL,
                                         NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_view),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  priv->manager = cm_manager_new (&error);
  if (error || !priv->manager) {
    g_debug ("Error initializing connman manager: %s\n",
             error->message);
    /* FIXME: must do better here */
    return;
  }
  cm_manager_refresh (priv->manager);
  priv->state = g_strdup (cm_manager_get_state (priv->manager));
  priv->icon_factory = carrick_icon_factory_new ();
  priv->icon = carrick_status_icon_new (priv->icon_factory,
                                        priv->manager);
  priv->pane = carrick_pane_new (priv->icon_factory,
                                 priv->manager);

  g_signal_connect (priv->manager,
                    "state-changed",
                    G_CALLBACK (manager_state_changed_cb),
                    self);
  g_signal_connect (priv->manager,
                    "connections-changed",
                    G_CALLBACK (manager_connections_changed_cb),
                    self);
}

CarrickApplet*
carrick_applet_new (void)
{
  return g_object_new (CARRICK_TYPE_APPLET, NULL);
}

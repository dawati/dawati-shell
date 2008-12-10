#include "moblin-netbook.h"

static void
on_notification_added (MoblinNetbookNotifyStore *store, 
                       Notification *n, 
                       gpointer foo)
{
  printf("[NOTIFICATION]: *new*: id:%i\n", n->id);

  printf("[NOTIFICATION]:      icon:%s\n", n->icon_name);
  printf("[NOTIFICATION]:   summary:%s\n", n->summary);
  printf("[NOTIFICATION]:      body:%s\n\n", n->body);
}

static void
on_notification_closed (MoblinNetbookNotifyStore *store, 
                        guint id, 
                        guint reason, 
                        gpointer foo)
{
  printf("[NOTIFICATION]: closed: %i, reason: %i\n", id, reason);
}

void
moblin_netbook_notify_init ()
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  if (priv->notify_store)
    return;

  priv->notify_store = moblin_netbook_notify_store_new ();

  g_signal_connect (priv->notify_store, 
                    "notification-added", 
                    G_CALLBACK (on_notification_added), NULL);
  g_signal_connect (priv->notify_store, 
                    "notification-closed", 
                    G_CALLBACK (on_notification_closed), NULL);
}

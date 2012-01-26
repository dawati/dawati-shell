#include <config.h>
#include <string.h>
#include <gconf/gconf-client.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "gsm-manager.h"
#include "gsm-manager-glue.h"
#include "gsm-presence.h"
#include "gsm-inhibitor.h"
#include "gsm-store.h"

#define GSM_MANAGER_DBUS_PATH "/org/gnome/SessionManager"

#define IS_STRING_EMPTY(x) ((x)==NULL||(x)[0]=='\0')

#define IDLE_KEY_DIR "/desktop/gnome/session"
#define IDLE_KEY IDLE_KEY_DIR "/idle_delay"

enum {
        INHIBITOR_ADDED,
        INHIBITOR_REMOVED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GsmManager, gsm_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSM_TYPE_MANAGER, GsmManagerPrivate))

struct _GsmManagerPrivate {
  DBusGConnection *connection;
  DBusGProxy *bus_proxy;
  GConfClient *gconf_client;
  GsmPresence *presence;
  GsmStore *inhibitors;
};

GQuark
gsm_manager_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("gsm_manager_error");
        }

        return ret;
}

#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
gsm_manager_error_get_type (void)
{
        static GType etype = 0;

        if (etype == 0) {
                static const GEnumValue values[] = {
                        ENUM_ENTRY (GSM_MANAGER_ERROR_GENERAL, "GeneralError"),
                        ENUM_ENTRY (GSM_MANAGER_ERROR_NOT_IN_INITIALIZATION, "NotInInitialization"),
                        ENUM_ENTRY (GSM_MANAGER_ERROR_NOT_IN_RUNNING, "NotInRunning"),
                        ENUM_ENTRY (GSM_MANAGER_ERROR_ALREADY_REGISTERED, "AlreadyRegistered"),
                        ENUM_ENTRY (GSM_MANAGER_ERROR_NOT_REGISTERED, "NotRegistered"),
                        ENUM_ENTRY (GSM_MANAGER_ERROR_INVALID_OPTION, "InvalidOption"),
                        { 0, 0, 0 }
                };

                g_assert (GSM_MANAGER_NUM_ERRORS == G_N_ELEMENTS (values) - 1);

                etype = g_enum_register_static ("GsmManagerError", values);
        }

        return etype;
}

static void
gsm_manager_dispose (GObject *object)
{
  G_OBJECT_CLASS (gsm_manager_parent_class)->dispose (object);
}

static void
gsm_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (gsm_manager_parent_class)->finalize (object);
}

static void
gsm_manager_class_init (GsmManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GsmManagerPrivate));

  object_class->dispose = gsm_manager_dispose;
  object_class->finalize = gsm_manager_finalize;

  signals [INHIBITOR_ADDED] =
    g_signal_new ("inhibitor-added",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GsmManagerClass, inhibitor_added),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE,
                  1, DBUS_TYPE_G_OBJECT_PATH);
  signals [INHIBITOR_REMOVED] =
    g_signal_new ("inhibitor-removed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GsmManagerClass, inhibitor_removed),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE,
                  1, DBUS_TYPE_G_OBJECT_PATH);

  dbus_g_object_type_install_info (GSM_TYPE_MANAGER, &dbus_glib_gsm_manager_object_info);
  dbus_g_error_domain_register (GSM_MANAGER_ERROR, NULL, GSM_MANAGER_TYPE_ERROR);
}

static void
on_idle_delay_changed (GConfClient *client,
                       guint        cnxn_id,
                       GConfEntry  *entry,
                       gpointer     user_data)
{
  GsmManager *manager = GSM_MANAGER (user_data);

  if (entry->value && entry->value->type == GCONF_VALUE_INT) {
    gsm_presence_set_idle_timeout (manager->priv->presence,
                                   gconf_value_get_int (entry->value) * 60000);
  }
}

static gboolean
inhibitor_has_flag (gpointer      key,
                    GsmInhibitor *inhibitor,
                    gpointer      data)
{
        guint flag;
        guint flags;

        flag = GPOINTER_TO_UINT (data);

        flags = gsm_inhibitor_peek_flags (inhibitor);

        return (flags & flag);
}

static gboolean
gsm_manager_is_idle_inhibited (GsmManager *manager)
{
        GsmInhibitor *inhibitor;

        if (manager->priv->inhibitors == NULL) {
                return FALSE;
        }

        inhibitor = (GsmInhibitor *)gsm_store_find (manager->priv->inhibitors,
                                                    (GsmStoreFunc)inhibitor_has_flag,
                                                    GUINT_TO_POINTER (GSM_INHIBITOR_FLAG_IDLE));
        if (inhibitor == NULL) {
                return FALSE;
        }
        return TRUE;
}

static void
update_idle (GsmManager *manager)
{
        if (gsm_manager_is_idle_inhibited (manager)) {
                gsm_presence_set_idle_enabled (manager->priv->presence, FALSE);
        } else {
                gsm_presence_set_idle_enabled (manager->priv->presence, TRUE);
        }
}

static void
on_store_inhibitor_added (GsmStore   *store,
                          const char *id,
                          GsmManager *manager)
{
        g_debug ("GsmManager: Inhibitor added: %s", id);
        g_signal_emit (manager, signals [INHIBITOR_ADDED], 0, id);
        update_idle (manager);
}

static void
on_store_inhibitor_removed (GsmStore   *store,
                            const char *id,
                            GsmManager *manager)
{
        g_debug ("GsmManager: Inhibitor removed: %s", id);
        g_signal_emit (manager, signals [INHIBITOR_REMOVED], 0, id);
        update_idle (manager);
}

typedef struct {
        const char *service_name;
        GsmManager *manager;
} RemoveClientData;

static gboolean
_debug_inhibitor (const char    *id,
                  GsmInhibitor  *inhibitor,
                  GsmManager    *manager)
{
        g_debug ("GsmManager: Inhibitor app:%s client:%s bus-name:%s reason:%s",
                 gsm_inhibitor_peek_app_id (inhibitor),
                 gsm_inhibitor_peek_client_id (inhibitor),
                 gsm_inhibitor_peek_bus_name (inhibitor),
                 gsm_inhibitor_peek_reason (inhibitor));
        return FALSE;
}

static void
debug_inhibitors (GsmManager *manager)
{
        gsm_store_foreach (manager->priv->inhibitors,
                           (GsmStoreFunc)_debug_inhibitor,
                           manager);
}

static gboolean
inhibitor_has_bus_name (gpointer          key,
                        GsmInhibitor     *inhibitor,
                        RemoveClientData *data)
{
        gboolean    matches;
        const char *bus_name_b;

        bus_name_b = gsm_inhibitor_peek_bus_name (inhibitor);

        matches = FALSE;
        if (! IS_STRING_EMPTY (data->service_name) && ! IS_STRING_EMPTY (bus_name_b)) {
                matches = (strcmp (data->service_name, bus_name_b) == 0);
                if (matches) {
                        g_debug ("GsmManager: removing inhibitor from %s for reason '%s' on connection %s",
                                 gsm_inhibitor_peek_app_id (inhibitor),
                                 gsm_inhibitor_peek_reason (inhibitor),
                                 gsm_inhibitor_peek_bus_name (inhibitor));
                }
        }

        return matches;
}

static void
remove_inhibitors_for_connection (GsmManager *manager,
                                  const char *service_name)
{
        RemoveClientData data;

        data.service_name = service_name;
        data.manager = manager;

        debug_inhibitors (manager);

        gsm_store_foreach_remove (manager->priv->inhibitors,
                                  (GsmStoreFunc)inhibitor_has_bus_name,
                                  &data);
}

static void
bus_name_owner_changed (DBusGProxy  *bus_proxy,
                        const char  *service_name,
                        const char  *old_service_name,
                        const char  *new_service_name,
                        GsmManager  *manager)
{
        if (strlen (new_service_name) == 0
            && strlen (old_service_name) > 0) {
                /* service removed */
                remove_inhibitors_for_connection (manager, old_service_name);
#if 0
                remove_clients_for_connection (manager, old_service_name);
#endif
        } else if (strlen (old_service_name) == 0
                   && strlen (new_service_name) > 0) {
                /* service added */

                /* use this if we support automatically registering
                 * well known bus names */
        }
}

static gboolean
register_manager (GsmManager *manager)
{
        GError *error = NULL;

        error = NULL;
        manager->priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (manager->priv->connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting session bus: %s", error->message);
                        g_error_free (error);
                }
                return FALSE;
        }

        manager->priv->bus_proxy = dbus_g_proxy_new_for_name (manager->priv->connection,
                                                              DBUS_SERVICE_DBUS,
                                                              DBUS_PATH_DBUS,
                                                              DBUS_INTERFACE_DBUS);
        dbus_g_proxy_add_signal (manager->priv->bus_proxy,
                                 "NameOwnerChanged",
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
                                 G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (manager->priv->bus_proxy,
                                     "NameOwnerChanged",
                                     G_CALLBACK (bus_name_owner_changed),
                                     manager,
                                     NULL);

        dbus_g_connection_register_g_object (manager->priv->connection, GSM_MANAGER_DBUS_PATH, G_OBJECT (manager));

        return TRUE;
}

static void
gsm_manager_init (GsmManager *manager)
{
	manager->priv = GET_PRIVATE (manager);

        manager->priv->gconf_client = gconf_client_get_default ();

        manager->priv->inhibitors = gsm_store_new ();
        g_signal_connect (manager->priv->inhibitors,
                          "added",
                          G_CALLBACK (on_store_inhibitor_added),
                          manager);
        g_signal_connect (manager->priv->inhibitors,
                          "removed",
                          G_CALLBACK (on_store_inhibitor_removed),
                          manager);

        manager->priv->presence = gsm_presence_new ();
#if 0
        /* Upstream this tells consolekit we're idle */
        g_signal_connect (manager->priv->presence,
                          "status-changed",
                          G_CALLBACK (on_presence_status_changed),
                          manager);
#endif

        gconf_client_add_dir (manager->priv->gconf_client,
                              IDLE_KEY_DIR,
                              GCONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);

        gconf_client_notify_add (manager->priv->gconf_client,
                                 IDLE_KEY,
                                 on_idle_delay_changed,
                                 manager,
                                 NULL,
                                 NULL);

        gconf_client_notify (manager->priv->gconf_client, IDLE_KEY);

        register_manager (manager);

        update_idle (manager);
}

static gboolean
_find_by_cookie (const char   *id,
                 GsmInhibitor *inhibitor,
                 guint        *cookie_ap)
{
        guint cookie_b;

        cookie_b = gsm_inhibitor_peek_cookie (inhibitor);

        return (*cookie_ap == cookie_b);
}

static guint32
generate_cookie (void)
{
        guint32 cookie;

        cookie = (guint32)g_random_int_range (1, G_MAXINT32);

        return cookie;
}

static guint32
_generate_unique_cookie (GsmManager *manager)
{
        guint32 cookie;

        do {
                cookie = generate_cookie ();
        } while (gsm_store_find (manager->priv->inhibitors, (GsmStoreFunc)_find_by_cookie, &cookie) != NULL);

        return cookie;
}

gboolean
gsm_manager_inhibit (GsmManager            *manager,
                     const char            *app_id,
                     guint                  toplevel_xid,
                     const char            *reason,
                     guint                  flags,
                     DBusGMethodInvocation *context)
{
        GsmInhibitor *inhibitor;
        guint         cookie;

        g_return_val_if_fail (GSM_IS_MANAGER (manager), FALSE);

        g_debug ("GsmManager: Inhibit xid=%u app_id=%s reason=%s flags=%u",
                 toplevel_xid,
                 app_id,
                 reason,
                 flags);

        if (IS_STRING_EMPTY (app_id)) {
                GError *new_error;

                new_error = g_error_new (GSM_MANAGER_ERROR,
                                         GSM_MANAGER_ERROR_GENERAL,
                                         "Application ID not specified");
                g_debug ("GsmManager: Unable to inhibit: %s", new_error->message);
                dbus_g_method_return_error (context, new_error);
                g_error_free (new_error);
                return FALSE;
        }

        if (IS_STRING_EMPTY (reason)) {
                GError *new_error;

                new_error = g_error_new (GSM_MANAGER_ERROR,
                                         GSM_MANAGER_ERROR_GENERAL,
                                         "Reason not specified");
                g_debug ("GsmManager: Unable to inhibit: %s", new_error->message);
                dbus_g_method_return_error (context, new_error);
                g_error_free (new_error);
                return FALSE;
        }

        if (flags == 0) {
                GError *new_error;

                new_error = g_error_new (GSM_MANAGER_ERROR,
                                         GSM_MANAGER_ERROR_GENERAL,
                                         "Invalid inhibit flags");
                g_debug ("GsmManager: Unable to inhibit: %s", new_error->message);
                dbus_g_method_return_error (context, new_error);
                g_error_free (new_error);
                return FALSE;
        }

        cookie = _generate_unique_cookie (manager);
        inhibitor = gsm_inhibitor_new (app_id,
                                       toplevel_xid,
                                       flags,
                                       reason,
                                       dbus_g_method_get_sender (context),
                                       cookie);
        gsm_store_add (manager->priv->inhibitors, gsm_inhibitor_peek_id (inhibitor), G_OBJECT (inhibitor));
        g_object_unref (inhibitor);

        dbus_g_method_return (context, cookie);

        return TRUE;
}

gboolean
gsm_manager_uninhibit (GsmManager            *manager,
                       guint                  cookie,
                       DBusGMethodInvocation *context)
{
        GsmInhibitor *inhibitor;

        g_return_val_if_fail (GSM_IS_MANAGER (manager), FALSE);

        g_debug ("GsmManager: Uninhibit %u", cookie);

        inhibitor = (GsmInhibitor *)gsm_store_find (manager->priv->inhibitors,
                                                    (GsmStoreFunc)_find_by_cookie,
                                                    &cookie);
        if (inhibitor == NULL) {
                GError *new_error;

                new_error = g_error_new (GSM_MANAGER_ERROR,
                                         GSM_MANAGER_ERROR_GENERAL,
                                         "Unable to uninhibit: Invalid cookie");
                dbus_g_method_return_error (context, new_error);
                g_debug ("Unable to uninhibit: %s", new_error->message);
                g_error_free (new_error);
                return FALSE;
        }

        g_debug ("GsmManager: removing inhibitor %s %u reason '%s' %u connection %s",
                 gsm_inhibitor_peek_app_id (inhibitor),
                 gsm_inhibitor_peek_toplevel_xid (inhibitor),
                 gsm_inhibitor_peek_reason (inhibitor),
                 gsm_inhibitor_peek_flags (inhibitor),
                 gsm_inhibitor_peek_bus_name (inhibitor));

        gsm_store_remove (manager->priv->inhibitors, gsm_inhibitor_peek_id (inhibitor));

        dbus_g_method_return (context);

        return TRUE;
}

gboolean
gsm_manager_is_inhibited (GsmManager *manager,
                          guint       flags,
                          gboolean   *is_inhibited,
                          GError     *error)
{
		GsmInhibitor *inhibitor;

        g_return_val_if_fail (GSM_IS_MANAGER (manager), FALSE);

        if (manager->priv->inhibitors == NULL
            || gsm_store_size (manager->priv->inhibitors) == 0) {
                *is_inhibited = FALSE;
                return TRUE;
        }

        inhibitor = (GsmInhibitor *) gsm_store_find (manager->priv->inhibitors,
                                                     (GsmStoreFunc)inhibitor_has_flag,
                                                     GUINT_TO_POINTER (flags));
        if (inhibitor == NULL) {
                *is_inhibited = FALSE;
        } else {
                *is_inhibited = TRUE;
        }

        return TRUE;
}

GsmManager*
gsm_manager_new (void)
{
  return g_object_new (GSM_TYPE_MANAGER, NULL);
}

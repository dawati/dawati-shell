#ifndef _GSM_MANAGER
#define _GSM_MANAGER

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define GSM_TYPE_MANAGER gsm_manager_get_type()

#define GSM_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSM_TYPE_MANAGER, GsmManager))

#define GSM_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GSM_TYPE_MANAGER, GsmManagerClass))

#define GSM_IS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSM_TYPE_MANAGER))

#define GSM_IS_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GSM_TYPE_MANAGER))

#define GSM_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSM_TYPE_MANAGER, GsmManagerClass))

typedef struct _GsmManagerPrivate GsmManagerPrivate;

typedef struct {
  GObject parent;
  GsmManagerPrivate *priv;
} GsmManager;

typedef struct {
  GObjectClass parent_class;
  void          (* inhibitor_added)     (GsmManager      *manager,
                                         const char      *id);
  void          (* inhibitor_removed)   (GsmManager      *manager,
                                         const char      *id);
} GsmManagerClass;

typedef enum
{
        GSM_MANAGER_ERROR_GENERAL = 0,
        GSM_MANAGER_ERROR_NOT_IN_INITIALIZATION,
        GSM_MANAGER_ERROR_NOT_IN_RUNNING,
        GSM_MANAGER_ERROR_ALREADY_REGISTERED,
        GSM_MANAGER_ERROR_NOT_REGISTERED,
        GSM_MANAGER_ERROR_INVALID_OPTION,
        GSM_MANAGER_NUM_ERRORS
} GsmManagerError;

GType gsm_manager_error_get_type (void);
#define GSM_MANAGER_TYPE_ERROR (gsm_manager_error_get_type ())

#define GSM_MANAGER_ERROR gsm_manager_error_quark ()
GQuark gsm_manager_error_quark (void);


GType gsm_manager_get_type (void);

GsmManager* gsm_manager_new (void);

gboolean            gsm_manager_inhibit                        (GsmManager            *manager,
                                                                const char            *app_id,
                                                                guint                  toplevel_xid,
                                                                const char            *reason,
                                                                guint                  flags,
                                                                DBusGMethodInvocation *context);
gboolean            gsm_manager_uninhibit                      (GsmManager            *manager,
                                                                guint                  inhibit_cookie,
                                                                DBusGMethodInvocation *context);
gboolean            gsm_manager_is_inhibited                   (GsmManager            *manager,
                                                                guint                  flags,
                                                                gboolean              *is_inhibited,
                                                                GError                *error);

G_END_DECLS

#endif /* _GSM_MANAGER */

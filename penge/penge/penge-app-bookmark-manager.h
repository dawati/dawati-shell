#ifndef _PENGE_APP_BOOKMARK_MANAGER
#define _PENGE_APP_BOOKMARK_MANAGER

#include <glib-object.h>

G_BEGIN_DECLS

#define PENGE_TYPE_APP_BOOKMARK_MANAGER penge_app_bookmark_manager_get_type()

#define PENGE_APP_BOOKMARK_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_APP_BOOKMARK_MANAGER, PengeAppBookmarkManager))

#define PENGE_APP_BOOKMARK_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_APP_BOOKMARK_MANAGER, PengeAppBookmarkManagerClass))

#define PENGE_IS_APP_BOOKMARK_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_APP_BOOKMARK_MANAGER))

#define PENGE_IS_APP_BOOKMARK_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_APP_BOOKMARK_MANAGER))

#define PENGE_APP_BOOKMARK_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_APP_BOOKMARK_MANAGER, PengeAppBookmarkManagerClass))

#define PENGE_TYPE_APP_BOOKMARK (penge_app_bookmark_get_type())

typedef struct
{
  gint ref_cnt;
  gchar *uri;
  gchar *application_name;
  gchar *icon_name;
  gchar *app_exec;
} PengeAppBookmark;

typedef struct {
  GObject parent;
} PengeAppBookmarkManager;

typedef struct {
  GObjectClass parent_class;
  void (*bookmark_added)(PengeAppBookmarkManager *manager, PengeAppBookmark *bookmark);
  void (*bookmark_removed) (PengeAppBookmarkManager *manager, const gchar *uri);
} PengeAppBookmarkManagerClass;

GType penge_app_bookmark_manager_get_type (void);
GType penge_app_bookmark_get_type (void);
PengeAppBookmark *penge_app_bookmark_new (void);
void penge_app_bookmark_free (PengeAppBookmark *bookmark);
void penge_app_bookmark_unref (PengeAppBookmark *bookmark);
PengeAppBookmark *penge_app_bookmark_ref (PengeAppBookmark *bookmark);

gboolean penge_app_bookmark_manager_add_from_uri (PengeAppBookmarkManager *manager,
                                                  const gchar             *uri,
                                                  GError                 **error_out);
gboolean penge_app_bookmark_manager_remove_by_uri (PengeAppBookmarkManager *manager,
                                                   const gchar             *uri,
                                                   GError                 **error_out);

GList *penge_app_bookmark_manager_get_bookmarks (PengeAppBookmarkManager *manager);

void penge_app_bookmark_manager_load (PengeAppBookmarkManager *manager);
void penge_app_bookmark_manager_save (PengeAppBookmarkManager *manager);


G_END_DECLS

#endif /* _PENGE_APP_BOOKMARK_MANAGER */


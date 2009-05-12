/* carrick-icon-factory.h */

#ifndef _CARRICK_ICON_FACTORY_H
#define _CARRICK_ICON_FACTORY_H

#include <gtk/gtk.h>
#include <gconnman/gconnman.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_ICON_FACTORY carrick_icon_factory_get_type()

#define CARRICK_ICON_FACTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CARRICK_TYPE_ICON_FACTORY, CarrickIconFactory))

#define CARRICK_ICON_FACTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CARRICK_TYPE_ICON_FACTORY, CarrickIconFactoryClass))

#define CARRICK_IS_ICON_FACTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CARRICK_TYPE_ICON_FACTORY))

#define CARRICK_IS_ICON_FACTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CARRICK_TYPE_ICON_FACTORY))

#define CARRICK_ICON_FACTORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CARRICK_TYPE_ICON_FACTORY, CarrickIconFactoryClass))

typedef enum {
  CARRICK_ICON_NO_NETWORK,
  CARRICK_ICON_NO_NETWORK_ACTIVE,
  CARRICK_ICON_ASSOCIATING,
  CARRICK_ICON_ASSOCIATING_ACTIVE,
  CARRICK_ICON_WIRED_NETWORK,
  CARRICK_ICON_WIRED_NETWORK_ACTIVE,
  CARRICK_ICON_WIRELESS_NETWORK_1,
  CARRICK_ICON_WIRELESS_NETWORK_1_ACTIVE,
  CARRICK_ICON_WIRELESS_NETWORK_2,
  CARRICK_ICON_WIRELESS_NETWORK_2_ACTIVE,
  CARRICK_ICON_WIRELESS_NETWORK_3,
  CARRICK_ICON_WIRELESS_NETWORK_3_ACTIVE
} CarrickIconState;

typedef struct {
  GObject parent;
} CarrickIconFactory;

typedef struct {
  GObjectClass parent_class;
} CarrickIconFactoryClass;

GType carrick_icon_factory_get_type (void);

CarrickIconFactory* carrick_icon_factory_new (void);

GdkPixbuf *carrick_icon_factory_get_pixbuf_for_service (CarrickIconFactory *factory,
                                                        CmService          *service);
GdkPixbuf *carrick_icon_factory_get_pixbuf_for_state (CarrickIconFactory *factory,
                                                      CarrickIconState    state);

CarrickIconState carrick_icon_factory_get_state_for_service (CmService *service);
const gchar *carrick_icon_factory_get_path_for_service (CmService *service);

G_END_DECLS

#endif /* _CARRICK_ICON_FACTORY_H */

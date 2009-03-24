#ifndef _DALSTON_BRIGHTNESS_SLIDER
#define _DALSTON_BRIGHTNESS_SLIDER

#include <glib-object.h>
#include <gtk/gtk.h>

#include <dalston/dalston-brightness-manager.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_BRIGHTNESS_SLIDER dalston_brightness_slider_get_type()

#define DALSTON_BRIGHTNESS_SLIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_BRIGHTNESS_SLIDER, DalstonBrightnessSlider))

#define DALSTON_BRIGHTNESS_SLIDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_BRIGHTNESS_SLIDER, DalstonBrightnessSliderClass))

#define DALSTON_IS_BRIGHTNESS_SLIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_BRIGHTNESS_SLIDER))

#define DALSTON_IS_BRIGHTNESS_SLIDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_BRIGHTNESS_SLIDER))

#define DALSTON_BRIGHTNESS_SLIDER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_BRIGHTNESS_SLIDER, DalstonBrightnessSliderClass))

typedef struct {
  GtkHScale parent;
} DalstonBrightnessSlider;

typedef struct {
  GtkHScaleClass parent_class;
} DalstonBrightnessSliderClass;

GType dalston_brightness_slider_get_type (void);

DalstonBrightnessSlider *dalston_brightness_slider_new (DalstonBrightnessManager *manager);

G_END_DECLS

#endif /* _DALSTON_BRIGHTNESS_SLIDER */


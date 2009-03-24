#include "dalston-brightness-manager.h"

#include <libhal-glib/hal-manager.h>
#include <libhal-glib/hal-device.h>

G_DEFINE_TYPE (DalstonBrightnessManager, dalston_brightness_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_BRIGHTNESS_MANAGER, DalstonBrightnessManagerPrivate))

typedef struct _DalstonBrightnessManagerPrivate DalstonBrightnessManagerPrivate;

struct _DalstonBrightnessManagerPrivate {
  HalManager *manager;
  HalDevice *panel_device;
  gchar *panel_udi;

  guint num_levels_discover_idle;

  gint num_levels;
};

enum
{
  NUM_LEVELS_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void
dalston_brightness_manager_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_brightness_manager_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_brightness_manager_dispose (GObject *object)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (object);

  if (priv->manager)
  {
    g_object_unref (priv->manager);
    priv->manager = NULL;
  }

  if (priv->panel_device)
  {
    g_object_unref (priv->panel_device);
    priv->panel_device = NULL;
  }

  if (priv->num_levels_discover_idle)
  {
    g_source_remove (priv->num_levels_discover_idle);
    priv->num_levels_discover_idle = 0;
  }

  G_OBJECT_CLASS (dalston_brightness_manager_parent_class)->dispose (object);
}

static void
dalston_brightness_manager_finalize (GObject *object)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (object);

  g_free (priv->panel_udi);

  G_OBJECT_CLASS (dalston_brightness_manager_parent_class)->finalize (object);
}

static void
dalston_brightness_manager_num_levels_changed (DalstonBrightnessManager *manager,
                                               gint                      num_levels)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (manager);

  priv->num_levels = num_levels;
}

static void
dalston_brightness_manager_class_init (DalstonBrightnessManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DalstonBrightnessManagerPrivate));

  object_class->get_property = dalston_brightness_manager_get_property;
  object_class->set_property = dalston_brightness_manager_set_property;
  object_class->dispose = dalston_brightness_manager_dispose;
  object_class->finalize = dalston_brightness_manager_finalize;

  klass->num_levels_changed = dalston_brightness_manager_num_levels_changed;

  signals[NUM_LEVELS_CHANGED] = 
    g_signal_new ("num-levels-changed",
                  DALSTON_TYPE_BRIGHTNESS_MANAGER,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET(DalstonBrightnessManagerClass, num_levels_changed),
                  0,
                  NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);
}

static gboolean
_num_levels_discover_idle_cb (gpointer data)
{
  DalstonBrightnessManager *manager = (DalstonBrightnessManager *)data;
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (data);
  gint value = -1;
  GError *error = NULL;

  if (!hal_device_get_int (priv->panel_device,
                           "laptop_panel.num_levels",
                           &value,
                           &error))
  {
    g_warning (G_STRLOC ": Error getting number of brightness levels: %s",
               error->message);
    g_clear_error (&error);
  } else {
    g_signal_emit (manager, signals[NUM_LEVELS_CHANGED], 0, value);
  }

  priv->num_levels_discover_idle = 0;

  return FALSE;
}


static void
dalston_brightness_manager_init (DalstonBrightnessManager *self)
{
  DalstonBrightnessManagerPrivate *priv = GET_PRIVATE (self);
  gchar **names;
  GError *error = NULL;

  priv->manager = hal_manager_new ();

  if (!hal_manager_find_capability (priv->manager,
                                    "laptop_panel",
                                    &names,
                                    &error))
  {
    g_warning (G_STRLOC ": Unable to find device with capability 'laptop_panel': %s",
               error->message);
    g_clear_error (&error);
    return;
  }

  priv->panel_udi = g_strdup (names[0]);
  hal_manager_free_capability (names);

  priv->panel_device = hal_device_new ();
  hal_device_set_udi (priv->panel_device, priv->panel_udi);

  priv->num_levels_discover_idle = g_idle_add (_num_levels_discover_idle_cb, self);
}

DalstonBrightnessManager *
dalston_brightness_manager_new (void)
{
  return g_object_new (DALSTON_TYPE_BRIGHTNESS_MANAGER, NULL);
}



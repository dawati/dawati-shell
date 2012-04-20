#include <meta/boxes.h>
#include <meta/meta-plugin.h>
#include <meta/screen.h>
#include <meta/display.h>
#include <meta/meta-background-actor.h>

#include "mutter-enum-types.h"

G_DEFINE_TYPE (MetaPlugin, meta_plugin, G_TYPE_OBJECT);


static void
meta_plugin_class_init (MetaPluginClass *klass)
{
}

static void
meta_plugin_init (MetaPlugin *self)
{
}


struct _MetaScreen
{
  GObject parent_instance;
};

struct _MetaScreenClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (MetaScreen, meta_screen, G_TYPE_OBJECT);

static void
meta_screen_class_init (MetaScreenClass *klass)
{
  g_signal_new ("restacked",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL, NULL,
                G_TYPE_NONE, 0);

  g_signal_new ("workspace-added",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL, NULL,
                G_TYPE_NONE,
                1,
                G_TYPE_INT);

  g_signal_new ("workspace-removed",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL, NULL,
                G_TYPE_NONE,
                1,
                G_TYPE_INT);
  g_signal_new ("workspace-switched",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL, NULL,
                G_TYPE_NONE,
                3,
                G_TYPE_INT,
                G_TYPE_INT,
                META_TYPE_MOTION_DIRECTION);

  g_signal_new ("window-entered-monitor",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL, NULL,
                G_TYPE_NONE, 2,
                G_TYPE_INT,
                META_TYPE_WINDOW);

  g_signal_new ("window-left-monitor",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL, NULL,
                G_TYPE_NONE, 2,
                G_TYPE_INT,
                META_TYPE_WINDOW);

  g_signal_new ("startup-sequence-changed",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL, NULL,
                G_TYPE_NONE, 1, G_TYPE_POINTER);

  g_signal_new ("toggle-recording",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL, NULL,
                G_TYPE_NONE, 0);

  g_signal_new ("workareas-changed",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL, NULL,
                G_TYPE_NONE, 0);

  g_signal_new ("monitors-changed",
                G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST,
                0,
                NULL, NULL, NULL,
                G_TYPE_NONE, 0);
}

static void
meta_screen_init (MetaScreen *self)
{
}


struct _MetaDisplay
{
  GObject parent_instance;
};

struct _MetaDisplayClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (MetaDisplay, meta_display, G_TYPE_OBJECT);

static void
meta_display_class_init (MetaDisplayClass *klass)
{
}

static void
meta_display_init (MetaDisplay *self)
{
}

struct _MetaWindow
{
  GObject parent_instance;
};

struct _MetaWindowClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (MetaWindow, meta_window, G_TYPE_OBJECT);

static void
meta_window_class_init (MetaWindowClass *klass)
{
}

static void
meta_window_init (MetaWindow *self)
{
}

struct _MetaWorkspace
{
  GObject parent_instance;
};

struct _MetaWorkspaceClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (MetaWorkspace, meta_workspace, G_TYPE_OBJECT);

static void
meta_workspace_class_init (MetaWorkspaceClass *klass)
{
}

static void
meta_workspace_init (MetaWorkspace *self)
{
}

G_DEFINE_TYPE (MetaWindowActor, meta_window_actor, CLUTTER_TYPE_ACTOR);

static void
meta_window_actor_class_init (MetaWindowActorClass *klass)
{
}

static void
meta_window_actor_init (MetaWindowActor *self)
{
}

G_DEFINE_TYPE (MetaBackgroundActor, meta_background_actor, CLUTTER_TYPE_ACTOR);

static void
meta_background_actor_class_init (MetaBackgroundActorClass *klass)
{
}

static void
meta_background_actor_init (MetaBackgroundActor *self)
{
}


struct _MetaKeyBinding
{
  guint dummy;
};

static void
meta_key_binding_free (MetaKeyBinding *binding)
{
  g_slice_free (MetaKeyBinding, binding);
}

static MetaKeyBinding *
meta_key_binding_copy (MetaKeyBinding *binding)
{
  return g_slice_dup (MetaKeyBinding, binding);
}

GType
meta_key_binding_get_type (void)
{
  static GType type_id = 0;

  if (G_UNLIKELY (type_id == 0))
    type_id = g_boxed_type_register_static (g_intern_static_string ("MetaKeyBinding"),
                                            (GBoxedCopyFunc)meta_key_binding_copy,
                                            (GBoxedFreeFunc)meta_key_binding_free);

  return type_id;
}

MetaRectangle *
meta_rectangle_copy (const MetaRectangle *rect)
{
  return g_memdup (rect, sizeof (MetaRectangle));
}

void
meta_rectangle_free (MetaRectangle *rect)
{
  g_free (rect);
}

GType
meta_rectangle_get_type (void)
{
  static GType type_id = 0;

  if (!type_id)
    type_id = g_boxed_type_register_static (g_intern_static_string ("MetaRectangle"),
					    (GBoxedCopyFunc) meta_rectangle_copy,
					    (GBoxedFreeFunc) meta_rectangle_free);
  return type_id;
}
